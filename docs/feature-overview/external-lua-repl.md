# External Lua REPL

The external Lua REPL lets a program outside the game run Lua in a started Lua mod and read back the result: the
return values, the `print` output, or the error. It is the REPL of the GUI's Lua Debugger tab without the GUI, meant
for development tools such as an editor integration, a test script, or a coding agent.

It is off by default.

> **Warning:** While it is enabled, any program that can write to the `LuaRepl` folder can run Lua in the game, with
> everything the targeted mod can do. There is no network listener, but do not share that folder over a network, and
> do not leave the setting on for normal play.

## Enabling it

In `UE4SS-settings.ini`:

```ini
[Debug]
ExternalLuaReplEnabled = 1
```

At startup UE4SS creates a `LuaRepl` folder in its working directory (the folder that `UE4SS.log` is written to) and
logs a warning naming it. Requests run from `UEngine::Tick`; in a game where UE4SS could not find that function the
feature stays off, request files are not read, and the log says so. UE4SS does not accept a `LuaRepl` that is a link or
a junction, and it skips request files that are links. Request ids are restricted so that no file name can point
outside the folder, but the folder is not a security boundary: anything that can write to it can already run Lua in
the game.

Unloading UE4SS with `FreeLibrary` while the feature is on is not supported.

## Sending a request

1. Pick an id of 8 to 32 lowercase hexadecimal characters, new for each request.
2. Write the request to `<id>.request.json.tmp` in the `LuaRepl` folder, close the file, and rename it to
   `<id>.request.json`. Renaming makes sure UE4SS never reads a half-written request.
3. Wait for `<id>.response.json` to appear, read it, and delete it.

UE4SS deletes a request file when it picks it up and runs each request file at most once; a new file with an id that
was used before runs again, so use a new id for every request. Files with other names are ignored.

### Request

```json
{"protocol": 1, "op": "exec", "mod": "MyMod", "code": "return FindFirstOf('PlayerController'):GetFullName()", "timeout_ms": 250}
```

| Field        | Meaning                                                                                                                                                                                                  |
|--------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `protocol`   | Required. Always `1`. A request with another number is answered with an error.                                                                                                                           |
| `op`         | `exec` runs `code` in `mod`. `list` answers with the names of the mods that `exec` can target and takes no other fields.                                                                                |
| `mod`        | For `exec`: the name of a started Lua mod, as shown in `mods.txt`.                                                                                                                                       |
| `code`       | For `exec`: Lua source. As in the Lua Debugger's REPL it is tried as an expression first (`return <code>`) and then as statements, so `1 + 1` returns `2`. Only source text is accepted, not bytecode. |
| `timeout_ms` | Optional. The time limit for this request, from 1 to 5000. Default 250. See [Time limit](#time-limit).                                                                                                  |

Fields that UE4SS does not know are ignored.

### Answer

```json
{"protocol": 1, "id": "3f9a1c0e5b7d2468", "ok": true, "error": "", "values": ["\"PlayerController /Game/Maps/Main.Main:PersistentLevel.PlayerController_0\""], "prints": [], "mods": [], "truncated": false, "elapsed_ms": 0.21}
```

| Field        | Meaning                                                                                                                                                                                                                               |
|--------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `ok`         | `true` if the code ran without an error.                                                                                                                                                                                              |
| `error`      | The error with a stack traceback, or the reason the request was refused (unknown mod, invalid JSON, too many requests waiting, ...). Empty when `ok` is `true`.                                                                     |
| `values`     | The returned values as display strings: strings in quotes, numbers as `tostring` writes them, tables with their number of entries (`100+` from 100 on), functions with their source line, UObjects with their full name or `(invalid)`, FNames with their text. No metamethod is called to produce them. |
| `prints`     | One entry for each `print` call made on the game thread while the request ran, including prints from hooks that the code triggered. The prints still go to the log as well.                                                        |
| `mods`       | For `list`: the names of the mods that `exec` can target.                                                                                                                                                                            |
| `truncated`  | `true` if something did not fit: at most 64 values, 4 KiB per value, 10000 prints with 64 KiB of text in total, and 16 KiB of error are kept.                                                                                     |
| `elapsed_ms` | How long the code ran.                                                                                                                                                                                                                |

Strings in the answer are valid UTF-8; bytes of a Lua string that are not are replaced with U+FFFD.

## How a request runs

- On the game thread, at the start of `UEngine::Tick`, at most one request per frame. Up to 8 requests wait; further
  ones are answered with an error until there is room.
- In the target mod's Lua state, on a Lua thread that is used only for these requests. The code sees the mod's
  globals and every UE4SS function, but not the `local` variables of the mod's scripts.
- A mod is never restarted or uninstalled while a request runs in it. If the code itself restarts its mod, for example
  with `RestartCurrentMod()`, the restart happens after the request has finished. While mods are being reinstalled,
  requests are answered with an error.
- Game-thread actions that the code causes, for example through `ExecuteInGameThread`, run after the request has
  finished, not inside it.
- Key binds that the code registers behave as if `main.lua` had registered them.
- Breakpoints of the Lua Debugger do not stop in request code. A breakpoint in another mod's hook that the code
  triggers, for example by calling a UFunction that mod hooks, pauses the game thread as it would outside a request.

## Time limit

Every 1000 Lua instructions the request checks whether it has run longer than `timeout_ms`. Once it has, it raises an
error before every further Lua instruction until the request ends, so code that catches the error with `pcall` in a
loop still stops. This turns an accidental endless loop into an error answer instead of a frozen game. It is a guard
against mistakes, not a sandbox:

- The check runs on the request's own Lua thread and in the coroutines the request creates. Lua that runs on other
  threads is not checked: coroutines that existed before the request, a request's coroutine that has since run outside
  it (it drops the check then), and hooks of other mods that the code triggers.
- The time counts from the start of the request and includes time spent in native calls, but it is only checked while
  Lua code runs. A single long native call is not interrupted.
- The time-limit error is not passed to `xpcall` message handlers.
- Lua runs finalizers (`__gc` metamethods) without the check.
- Code that calls `debug.sethook()` removes the check.

The code has the full power of the mod it runs in and follows the same rules as any mod. For example, passing a Lua
string or `nil` where a UFunction expects an `FName`, or using a UObject that has been destroyed, can crash the game.

## Example client

A minimal client in Python:

```python
import json
import os
import secrets
import time
from pathlib import Path

def run_lua(repl_folder, mod, code, timeout_ms=250, wait_seconds=5.0):
    folder = Path(repl_folder)
    request_id = secrets.token_hex(8)
    request = {"protocol": 1, "op": "exec", "mod": mod, "code": code, "timeout_ms": timeout_ms}

    temporary = folder / f"{request_id}.request.json.tmp"
    temporary.write_text(json.dumps(request), encoding="utf-8")
    os.replace(temporary, folder / f"{request_id}.request.json")

    answer_path = folder / f"{request_id}.response.json"
    deadline = time.monotonic() + wait_seconds
    while time.monotonic() < deadline:
        try:
            answer = json.loads(answer_path.read_text(encoding="utf-8"))
        except (FileNotFoundError, PermissionError):
            time.sleep(0.01)
            continue
        answer_path.unlink()
        return answer
    raise TimeoutError(f"no answer to request {request_id}")

print(run_lua(r"C:\Games\MyGame\MyGame\Binaries\Win64\ue4ss\LuaRepl", "MyMod", "1 + 1")["values"])  # ['2']
```
