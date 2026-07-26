# Palworld Linux Offline Signature Report

Pinned target:

- Steam app ID: `2394010`
- Steam build ID: `24088465`
- Executable size: `196281496` bytes
- SHA-256: `a05788ead7619db22a1509c43241c16d289ed7040e8bcbb2e36e13e223e822f9`
- ELF build ID: `217802a00653a9c4`
- Embedded engine branch: `++UE5+Release-5.1`
- Embedded build date: `Jun 22 2026`

Reproduce the report from an anonymous SteamCMD installation:

```bash
tools/linux/extract-palworld-signature-report.sh "$PALWORLD_SERVER_ROOT"
PALWORLD_SERVER_ROOT="$PALWORLD_SERVER_ROOT" \
  ctest --test-dir build_linux -R '^PalworldSignatureTests$' -V
```

Resolver results for the pinned executable:

| Resolver | Result |
|---|---:|
| `EngineVersion` | `5.1` |
| `GUObjectArray` | `0x0c11d878` |
| `FNameCtorWchar` | `0x079409c0` |
| `FNameToString` | `0x07941e40` |
| `GMalloc` | `0x0c0482a8` |
| `StaticConstructObjectInternal` | `0x07b851c0` |
| `FTextFString` | `0x0781f080` |
| `GNatives` | `0x0c11c0a0` |
| `ConsoleManagerSingleton` | `0x077ba9d0` |
| `UGameEngineTick` | `0x0a3928b0` |
| `FUObjectHashTablesGet` | optional miss |

`FUObjectHashTablesGet` is deliberately not substituted with an unrelated address. The Palworld Linux shipping build inlines the relevant access, and the current UE4SS binding marks this resolver optional and does not assign its result to runtime state. All addresses that the Linux runtime consumes, plus the console-manager and engine-tick probes needed by the acceptance suite, resolve in the offline regression.

`UObject::ProcessEvent` is recovered from the UE 5.1 `UObject` vtable after live objects exist, not through patternsleuth. Its approved UE 5.1 vtable offset is `0x260`; the live acceptance harness remains responsible for proving that hook.
