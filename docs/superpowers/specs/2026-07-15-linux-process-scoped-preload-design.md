# Linux Process-Scoped Preload Design

**Status:** Approved and reviewed on 2026-07-15; implemented on the `linux-port` branch.

## Context

The native Linux launcher loads UE4SS by prepending `libUE4SS.so` to `LD_PRELOAD` and then executing the requested command. Unlike a Windows proxy DLL or targeted `LoadLibrary` injection, `LD_PRELOAD` is inherited through the process environment. Every descendant created with `exec` therefore loads UE4SS unless the parent removes it from the environment first.

Palworld starts `crashpad_handler` after the server has initialized. The helper currently inherits both `LD_PRELOAD` and `UE4SS_MODULE_PATH`, starts a second UE4SS instance, loads probe mods, scans a non-Unreal executable until timeout, and writes into the same staged log directory. This produces false acceptance failures and can overwrite the real server's `UE4SS.log` even though the server itself reaches its ready marker.

Windows does not require this policy. Its standard proxy and manual injection paths load `UE4SS.dll` into one selected process, and loaded modules are not inherited by child processes. The design below is Linux-only and must leave `EntryWin32.cpp` and Windows proxy behavior unchanged.

## Goals

- Start UE4SS only in the intended Unreal host when using the supported Linux launcher.
- Prevent descendants of the accepted host from inheriting the launcher-added UE4SS entry in `LD_PRELOAD`.
- Support direct ELF launches and shell wrappers such as `PalServer.sh`.
- Preserve any pre-existing user `LD_PRELOAD` entries exactly.
- Preserve marker-free raw `LD_PRELOAD` and explicit `dlopen` behavior for compatibility.
- Decide whether to start before creating UE4SS threads, output devices, logs, or mods.
- Keep paths containing spaces and the existing `/proc/self/fd` preload fallback working.

## Non-goals

- System-wide injection or attach-style injection.
- Preventing users from deliberately preloading UE4SS into multiple processes.
- Making marker-free raw `LD_PRELOAD` process-scoped.
- Adding helper-specific deny lists such as a `crashpad_handler` basename check.
- Changing Windows bootstrap behavior.

## Launcher interface

The existing direct-launch form remains valid:

```bash
run_ue4ss.sh /path/to/PalServer-Linux-Shipping Pal -port=8211
```

For a wrapper command, the launcher accepts an explicit host executable:

```bash
run_ue4ss.sh \
  --host-executable /path/to/Pal/Binaries/Linux/PalServer-Linux-Shipping \
  /path/to/PalServer.sh -port=8211
```

The first non-option argument is always the command to execute. If `--host-executable` is omitted, the command itself is the expected host. An omitted host is accepted only when the command is an ELF executable. A script command without `--host-executable` fails before launch with a message explaining the required form. This avoids silently running an unmodded game because `/proc/self/exe` identifies the script interpreter rather than the script path.

The launcher validates that the command and expected host exist and are executable. The expected host must be a regular ELF file. It canonicalizes their paths before exporting the private activation contract.

## Private launcher contract

The launcher records:

- the canonical expected host executable;
- the exact original `LD_PRELOAD` value;
- whether `LD_PRELOAD` was originally present, which distinguishes unset from set-to-empty;
- the canonical UE4SS module path already used by the runtime.

The internal environment variable names are:

- `UE4SS_LAUNCH_TARGET_EXE`
- `UE4SS_LAUNCH_LD_PRELOAD_WAS_SET`
- `UE4SS_LAUNCH_ORIGINAL_LD_PRELOAD`
- `UE4SS_MODULE_PATH`

These variables are an implementation detail of `run_ue4ss.sh`, not a second public launcher API. The launcher overwrites stale values from an ancestor launcher invocation.

After recording the original environment, the launcher prepends UE4SS to `LD_PRELOAD` exactly as it does today. If the library path contains whitespace or a colon, the existing open-file-descriptor `/proc/self/fd/<n>` form remains the preload entry while `UE4SS_MODULE_PATH` retains the canonical filesystem path.

## Constructor activation policy

The Linux shared-object constructor evaluates startup in this order:

1. If `UE4SS_DISABLE_AUTO_START=1`, return immediately. This remains the highest-priority override.
2. If no launcher target marker exists, use the current legacy behavior: apply the atomic once guard and start UE4SS. No environment restoration occurs in this compatibility mode.
3. If a launcher target marker exists, resolve the current process identity from `/proc/self/exe`.
4. Compare the current executable and expected host using filesystem equivalence, so canonical paths, symlinks, and hard links identify the same file.
5. If they do not match, return before touching the once guard, creating a thread, opening a log, or loading a mod. Leave the environment unchanged so a later descendant can still reach the expected host.
6. If they match, capture the UE4SS module path, restore the original `LD_PRELOAD`, remove the private launcher variables and `UE4SS_MODULE_PATH`, then apply the once guard and start UE4SS with the captured module path.

Restoring the environment in the accepted host is essential. The library remains mapped in that process, but future descendants inherit only the user's original preload configuration. In the normal launcher workflow, where the original value did not already contain UE4SS, helpers therefore do not load UE4SS at all. If the user deliberately included UE4SS in the original value, exact preservation takes precedence and descendants retain that user-requested preload. Avoiding the launcher-added load is still safer than loading the library into every helper and merely skipping its initialization.

## Wrapper-process flow

```text
run_ue4ss.sh
  -> /bin/sh PalServer.sh
       UE4SS constructor: target mismatch, skip and preserve environment
       -> PalServer-Linux-Shipping
            UE4SS constructor: target match
            restore original LD_PRELOAD and start UE4SS
            -> crashpad_handler
                 inherits restored environment; UE4SS is not loaded
```

This supports Palworld's current wrapper, which launches the server as a child rather than replacing the shell with `exec`.

## Error handling and diagnostics

When a launcher marker is present, startup fails closed if:

- the expected-host marker is empty or malformed;
- `/proc/self/exe` cannot be resolved;
- filesystem equivalence cannot be determined;
- restoring or removing the private environment state fails.

Fail-closed means that UE4SS does not initialize, while the host program continues normally. A concise message is written to stderr. With `UE4SS_DIAGNOSE=1`, skipped wrapper processes may emit:

```text
DIAG: startup_skipped executable=<path> expected=<path> reason=target_mismatch
```

This diagnostic must not use `inactive_reason`, because a normal wrapper mismatch is not an initialization failure and must not trigger the acceptance harness's crash detection.

The accepted target emits its normal executable and signature diagnostics. Child helpers created after restoration emit no UE4SS diagnostics because the library is no longer present in their `LD_PRELOAD` environment.

## Compatibility

- Existing direct launcher calls remain source-compatible.
- Wrapper scripts require the new `--host-executable` option.
- Existing `LD_PRELOAD` entries remain in their original order and exact value after the target accepts UE4SS.
- If the original `LD_PRELOAD` already contains UE4SS, preserving it can intentionally keep UE4SS in descendants; process scoping covers the entry added by the launcher.
- Marker-free raw preload continues to start UE4SS exactly as it does today, including its existing child-inheritance behavior. Documentation identifies the launcher as the supported process-scoped workflow.
- Explicit `dlopen` without launcher markers remains unchanged.
- `UE4SS_DISABLE_AUTO_START=1` remains unchanged.
- Windows files and behavior remain unchanged.

## Implementation boundaries

The change is limited to:

- `tools/linux/run_ue4ss.sh` for argument parsing, validation, canonicalization, and environment capture;
- `UE4SS/src/Platform/Linux/EntryLinux.cpp` plus a small Linux startup-policy helper if extraction materially improves unit testing;
- Linux launcher/startup tests;
- `docs/linux.md`, the README launcher section, and the active Palworld acceptance plan or verification report where process scoping is described.

No helper name, Palworld path, or crashpad-specific behavior belongs in the runtime policy.

## Test strategy

Implementation follows strict RED/GREEN TDD.

### Launcher tests

- Direct ELF invocation selects the command as the default host.
- `--host-executable` preserves command arguments and supports a wrapper script.
- A script without an explicit host is rejected with actionable output.
- Missing, non-executable, non-regular, and non-ELF host paths fail before execution.
- Existing `LD_PRELOAD` values, including multiple entries, are captured without reordering or loss.
- Paths containing spaces continue to use the file-descriptor preload path.

### Startup-policy tests

- Marker absent selects legacy startup.
- `UE4SS_DISABLE_AUTO_START=1` overrides every other state.
- Marker present plus equivalent executable selects startup.
- Marker present plus a different executable skips before the once guard.
- Symlink and hard-link host identities compare as equivalent.
- Malformed markers and executable-resolution failures fail closed.
- Accepted startup restores unset, empty, and non-empty original `LD_PRELOAD` states exactly.
- Accepted startup removes all private launcher variables before descendants are created.

### Process integration tests

- A wrapper process receives UE4SS but does not initialize it.
- The designated descendant host initializes UE4SS exactly once.
- A helper spawned by the host observes the original `LD_PRELOAD` and does not load UE4SS.
- Marker-free raw preload retains the current behavior.

### Palworld acceptance

- The server reaches its ready marker through `run_ue4ss.sh`.
- The log contains exactly one main executable diagnostic for `PalServer-Linux-Shipping`.
- No UE4SS executable diagnostic, signature scan, mod marker, timeout, or fatal error is emitted by `crashpad_handler`.
- Lua and C++ probe lifecycle markers remain present.
- The protected unrelated server process is never signalled or modified.

## Documentation requirements

`docs/linux.md` and the README will document:

- direct ELF launcher syntax;
- wrapper syntax with `--host-executable`;
- why Linux child processes inherit `LD_PRELOAD` while Windows proxy injection does not;
- that the launcher restores the user's original preload configuration before the game starts helpers;
- that raw manual preload remains compatible but is not process-scoped;
- the `startup_skipped` diagnostic and common host-marker mistakes.

The final Palworld verification report will include the command used, the single main-process diagnostic, and evidence that crashpad produced no UE4SS startup output.
