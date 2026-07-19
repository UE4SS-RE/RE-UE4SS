# Box64 Orphan Preload Fail-Safe Design

**Status:** Approved direction on 2026-07-19; ready for implementation planning.

## Context

PR #1342 scopes Linux launcher injection by exporting `UE4SS_LAUNCH_TARGET_EXE`, comparing it with the current executable, and restoring the original `LD_PRELOAD` before the selected Unreal process starts children. Marker-free `LD_PRELOAD` and explicit `dlopen` retain their legacy behavior.

Box64 can run x86-64 Linux programs on ARM64 and has a separate `BOX64_LD_PRELOAD` list for x86-64 guest libraries. An ARM64 Palworld user reported that `crashpad_handler` can receive `libUE4SS.so` through this Box64 list after the selected server has removed the UE4SS launcher markers. The startup policy then sees no target marker, chooses `LegacyStart`, and initializes UE4SS in the non-Unreal helper. The helper repeatedly scans for engine signatures, consumes CPU, and writes noise to the server's shared log.

PR #1342 targets native Linux. This change adds a narrow safety guard for accidental Box64 reinjection; it does not make Box64 a supported platform.

## Goals

- Skip UE4SS initialization when the target marker is absent and `BOX64_LD_PRELOAD` identifies the currently loaded UE4SS library.
- Make the decision before UE4SS creates threads, output devices, logs, or mods.
- Preserve marker-free raw `LD_PRELOAD`, explicit `dlopen`, and the existing launcher contract on native Linux.
- Avoid helper-specific names, Palworld paths, new launcher options, and Box64 environment mutation.
- Document Box64 as best-effort and unsupported.

## Non-goals

- Full ARM64 or Box64 support.
- A public `--box64` launcher mode.
- Restoring or rewriting `BOX64_LD_PRELOAD`.
- Guaranteeing support for deliberate marker-free UE4SS loading through `BOX64_LD_PRELOAD`.
- Detecting every emulator or binary-translation environment.
- Adding a `crashpad_handler` deny list.

## Startup policy

The Linux constructor resolves the path of its own loaded shared object with `dladdr` and passes it to the startup-policy evaluator. The evaluator keeps the existing marker-first behavior:

1. If `UE4SS_LAUNCH_TARGET_EXE` is present, use the existing target comparison and launcher-state validation unchanged.
2. If the target marker is absent, inspect `BOX64_LD_PRELOAD`.
3. Split the Box64 value on colons, matching Box64's own list parser and ignoring empty entries.
4. Treat an entry as UE4SS when it is the same path, is filesystem-equivalent to the loaded module, or has the same filename as the loaded module. Filename comparison covers Box64 values such as `libUE4SS.so` that are resolved through its guest library path.
5. If a matching entry exists, return a distinct `Box64OrphanedPreload` decision with reason `box64_target_missing`.
6. Otherwise return the existing `LegacyStart` decision.

The constructor handles `Box64OrphanedPreload` like a normal target mismatch: it returns before module initialization and emits only a `startup_skipped` line when `UE4SS_DIAGNOSE=1`. It does not emit an inactive reason or create `UE4SS.log`.

The guard deliberately fails closed for marker-free loading through `BOX64_LD_PRELOAD`. Without a target marker, UE4SS cannot distinguish the intended Box64 guest from a reinjected helper. Users needing reliable process selection must use a native x86-64 host and the supported launcher contract.

## Compatibility

- A missing target marker with no `BOX64_LD_PRELOAD` remains `LegacyStart`.
- A missing target marker with an unrelated Box64 preload list remains `LegacyStart`.
- Marker-present launcher decisions take precedence over the Box64 guard.
- Native launcher behavior and environment restoration do not change.
- Windows behavior does not change.
- No public launcher arguments or environment variables are added.

## Error handling and diagnostics

The fail-safe is conservative. Empty Box64 entries, nonexistent paths, and filesystem-equivalence errors do not independently disable startup; filename and lexical comparisons still apply. Only a positive match against the loaded UE4SS module triggers the guard.

With diagnostics enabled, the skipped process reports:

```text
DIAG: startup_skipped executable=<path> reason=box64_target_missing
```

The diagnostic is written to stderr only. Normal operation remains silent.

## Testing

Unit tests will cover:

- A marker-free process with no Box64 variable remains legacy.
- An empty or unrelated Box64 list remains legacy.
- Exact, basename-only, symlink, and hard-link UE4SS entries trigger `Box64OrphanedPreload`.
- Colon-separated lists detect a matching entry in any position.
- A valid launcher target still produces `LauncherStart` even when the Box64 list names UE4SS.

The process-scope integration test will add an orphan-helper invocation with the target marker removed and `BOX64_LD_PRELOAD` naming the staged UE4SS library. It must exit without creating a second log or scan marker and must emit the diagnostic only when diagnostics are enabled.

Targeted Linux startup, launcher, preload-scope, and fail-soft tests will run before the complete CTest suite.

## Documentation

`docs/linux.md` will state that native x86-64 Linux is the supported target. Box64 is best-effort: the runtime blocks a recognizable orphaned Box64 preload to protect helpers, but this is not complete Box64 support and marker-free `BOX64_LD_PRELOAD` injection is intentionally rejected.
