# Palworld Cross-Platform Building Restrictions Mod Design

**Date:** 2026-07-15

**Status:** Approved for autonomous implementation on 2026-07-15

**Phase 1 targets:** Steam Windows client and native Linux dedicated server

**Deferred target:** Windows dedicated server

## Context

The reference `BuildingRestrictionsDisabler` release contains a native Windows C++ UE4SS DLL, a Windows .NET standalone process patcher, and Cheat Engine tables for Steam, Game Pass, and Windows dedicated-server processes. The DLL is not C# and cannot be recompiled for Linux. Its public repository contains no implementation source or declared license, so no code, encrypted resource, or patch database from the reference release will be copied into this repository.

The replacement will be a clean-room UE4SS C++ mod based on observable behavior and independently derived signatures from user-supplied game binaries. The Cheat Engine tables and standalone configuration identify restriction concepts, but their byte patterns are reference material only. The supplied table targets an older build: only 38 of 54 Steam patterns matched the updated client uniquely as written.

Phase 1 is pinned to these local targets:

| Target | Identity | Notes |
|---|---|---|
| Steam Windows client | SHA-256 `5b669bce7b8546ae27454d7a9d8cf0815e41c96b1df488b5dc435cbac60fffaf`; PE preferred base `0x7ff7aa200000` | The supplied JMAP resolves to valid native functions in this executable. |
| Linux dedicated server | Palworld `v1.0.1.100619`; Steam build `24181105`; SHA-256 `788649fa1592160faa7bcf07ccd16d474ebeaae954717bc32284b5a43028d8e7`; ELF build ID `7f7e167407984ec3` | Installed under `build_linux/palworld-server`; the imported save and settings are preserved. |

The updated Linux server reaches its ready marker with UE4SS, but the pre-update Palworld signature test rejects the new executable size and UE4SS reports a missing `FUObjectHashTables::Get` pattern. Refreshing that baseline is a prerequisite to clean live-mod acceptance.

## Goals

- Produce one source project that builds a Windows `main.dll` and Linux `main.so` UE4SS C++ mod.
- Relax building restrictions on both the Windows placement client and authoritative Linux server.
- Apply configuration once during startup; changing configuration requires restarting the process.
- Provide a master switch and an individual toggle for every supported restriction.
- Use conservative defaults that retain ownership, administrative-limit, resource-node, and high-risk collision protections.
- Validate the exact executable build, every signature, match count, and original patch bytes before modifying executable memory.
- Let one unsupported restriction fail independently without silently disabling unrelated restrictions.
- Fail closed on unknown builds, malformed configuration, missing matches, ambiguous matches, or unexpected original bytes.
- Package both platform libraries and documented configuration in one release layout.
- Make adding a Windows dedicated-server profile later a data/profile change rather than a core redesign.

## Non-goals

- Recreating the reference DLL's D3D overlay, update checker, configuration UI, or F8/F9 live toggles.
- Runtime patch removal or configuration reload.
- Windows Game Pass support in phase 1.
- Windows dedicated-server support in phase 1.
- Redistributing Palworld binaries, JMAP files, Cheat Engine tables, the standalone executable, or the reference DLL.
- Guaranteeing compatibility with unrecognized future Palworld builds.
- Bypassing material costs, technology unlocks, guild ownership, or unrelated gameplay rules unless a named toggle explicitly says so.

## Considered approaches

### 1. One cross-platform native UE4SS C++ mod — selected

A shared configuration and patch engine consumes separate Windows-client and Linux-server profiles. The approach matches UE4SS's native mod lifecycle, keeps game-specific behavior outside the UE4SS core, and prevents the two platform implementations from drifting.

### 2. High-level Unreal reflection hooks

Reflection hooks would be less dependent on machine code, but many placement decisions are private native paths rather than exported `UFunction` calls. The JMAP supplies useful semantic anchors, not complete hook coverage. This approach cannot reliably reproduce the requested restriction set.

### 3. Separate Windows and Linux patchers

A Windows CT-style patcher plus a Linux preload patcher would be initially direct, but would duplicate configuration, diagnostics, safety validation, packaging, and tests. It also would not behave as one mod across targets.

## Repository and build structure

The mod lives under `cppmods/BuildingRestrictionsDisabler` and contains:

- a platform-neutral static core for configuration, build identity, pattern scanning, patch planning, and result reporting;
- a UE4SS-facing shared-library target exporting `start_mod` and `uninstall_mod`;
- small Windows and Linux backends for module discovery, page protection, and instruction-cache flushing;
- compiled-in target profiles kept separate from the patch engine;
- focused unit and offline integration tests registered under `tests`.

The root CMake build will not enable the existing GUI C++ mods on Linux. It will add only the headless building-restrictions target behind a focused option. Windows builds link the mod and UE4SS in the same configuration so the C++ runtime and UE4SS ABI match. Linux builds do the same with Clang and `libUE4SS.so`. Existing xwin tooling produces the Windows DLL from Linux; a native Windows/MSVC build remains a supported validation path.

The packaged mod directory is:

```text
BuildingRestrictionsDisabler/
├── enabled.txt
├── config.json
└── dlls/
    ├── main.dll
    └── main.so
```

UE4SS selects `main.dll` on Windows and `main.so` on Linux. The unused platform file is inert.

## Components and boundaries

### Configuration

`config.json` has a master `enabled` boolean and a `restrictions` object. A restriction value of `true` means "disable this game restriction and allow the placement." This meaning is stated at the top of the shipped file and in documentation.

Configuration is loaded exactly once. A missing file uses compiled conservative defaults and logs that decision. A syntactically invalid file, wrong value type, or contradictory schema fails closed and applies no patches. Unknown keys are reported and ignored so newer configuration files do not silently change older builds.

The initial schema is conceptually:

```json
{
  "enabled": true,
  "restrictions": {
    "overlaps_another_object": false,
    "not_placed_on_ground": true,
    "all_floors_touch_ground": true,
    "too_close_another_base": false,
    "too_close_boss_or_facility": false,
    "below_sea_level": true,
    "on_oil_resource": false,
    "high_location": true,
    "max_structures_per_base": false,
    "in_dungeon": true,
    "ground_slope": true,
    "max_bases": false,
    "within_base_range": true,
    "obstacle_in_way": true,
    "indoors": true,
    "other_guild_base": false,
    "connected_to_structure": true,
    "enough_support": true,
    "place_wall": true,
    "place_ceiling": true,
    "prevent_destroy_after_placement": false,
    "disable_snapping": false,
    "steep_angle_rotation": false
  }
}
```

Profiles may mark a restriction as unsupported on a target. An enabled but unsupported restriction is not approximated by another patch; it is reported explicitly.

### Build identity

Profile selection occurs before scanning restriction patterns:

- Windows uses PE machine type, timestamp, image size, file size, and a deterministic executable-code fingerprint.
- Linux uses ELF machine type, GNU build ID, image size, file size, and a deterministic executable-code fingerprint.

The code fingerprint covers executable sections/segments and is implemented identically in offline tests and runtime module inspection. Full profile identity must match. Pattern matches are validation, not a substitute for build identity.

### Restriction registry

Each restriction has a stable identifier, user-facing description, conservative default, supported target roles, and one or more patch sites per profile. Each patch site defines:

- a byte pattern with explicit wildcards for relocations and unstable call displacements;
- an executable-segment scan scope;
- the exact expected match count;
- a patch offset relative to each match;
- expected original bytes at the patch offset;
- replacement bytes.

The shared scanner does not search writable heaps or unrelated shared libraries. Windows scans executable PE sections in the main module. Linux scans executable `PT_LOAD` segments for the main ELF image.

### Patch planning and application

Patching is transactional per restriction:

1. Resolve the active profile and load configuration.
2. Scan every site belonging to one enabled restriction.
3. Require exact match counts and validate every original byte.
4. If any site fails, apply none of that restriction's sites.
5. If all sites validate, produce a complete patch plan.
6. Acquire write permission for every affected page before the first write. If acquisition fails, restore any page already changed and apply no bytes.
7. Apply all planned bytes, restore every page's original protection, and flush the instruction cache.
8. Log the restriction result and continue with the next restriction.

This permits an independent restriction to fail without creating a half-applied implementation of that same restriction. A protection-acquisition failure occurs before writes. A protection-restore or cache-flush failure after writes is fatal for the patching session: it is logged clearly and no later restriction is attempted.

The patcher recognizes both original and already-patched byte states. Re-loading an already-applied profile is idempotent. It never restores original bytes during hot unload because executing threads could race with restoration. `uninstall_mod` releases mod-owned objects but leaves startup patches in place until process exit. Documentation states that uninstall, updates, and configuration changes require a full restart.

### Platform backends

The Windows backend uses the main PE module, `VirtualProtect`, and `FlushInstructionCache`. The Linux backend uses `dl_iterate_phdr`/the main ELF program headers, `mprotect`, and `__builtin___clear_cache`. Both expose the same narrow interface to the patch engine:

- describe executable regions;
- report build identity inputs;
- change and restore page protections;
- flush an address range after modification.

No platform conditional belongs in restriction definitions or configuration parsing.

### UE4SS lifecycle

The mod loads configuration and applies patches once during native mod startup, before players can perform placement actions. It does not register UI tabs, keyboard handlers, update callbacks, or network services. It logs a final summary containing the selected profile, enabled count, applied count, unsupported count, and failed count.

## Target profile derivation

### Windows client

The JMAP contains Palworld classes, vtables, and native `UFunction` entry points that resolve inside the supplied PE image. It is used to name and anchor relevant building systems such as `PalBuildObjectInstallChecker`, `PalBuildObjectOverlapChecker`, `PalBuildObjectInstallStrategyBase`, and `PalBuilderComponent`.

The reference CT table provides semantic labels and candidate instruction neighborhoods. Exact table patterns are not committed. Every Windows patch is re-derived from the updated PE disassembly, widened around unstable relative calls, and tested for uniqueness against the complete executable. Short ambiguous patterns such as a reason-code move followed only by `E8`/`E9` require surrounding control-flow context.

### Linux server

Linux signatures are derived independently from the updated ELF. Windows offsets and instruction sequences are never assumed to transfer across the MSVC and Linux compiler outputs. Cross-platform reasoning uses shared data flow, reason-code constants, strings, call relationships, and Unreal type names, followed by Linux disassembly and live validation.

Before mod acceptance, the repository's pinned Palworld baseline is refreshed for Steam build `24181105`, including the stale executable identity and an audit of the missing optional `FUObjectHashTables::Get` result. If the helper remains inlined in this build, tests and documentation must identify that state explicitly rather than inventing an unsafe signature. Every required UE4SS startup signature must pass without weakening exact-build checks.

### Future Windows server

The later Windows-server phase adds another compiled profile and offline binary test. It may reuse restriction definitions and patch semantics, but it receives independent build identity, scan patterns, counts, and original-byte validation. No phase-1 code path assumes that the Windows client and server binaries are identical.

## Error handling and diagnostics

The mod never guesses after a validation failure. It emits concise structured messages for:

- unsupported process role or platform;
- unknown build identity;
- missing, malformed, or unknown configuration entries;
- unsupported configured restriction;
- zero, too few, or too many signature matches;
- unexpected original bytes;
- page-protection or cache-flush failure;
- successful, already-applied, skipped, and failed restrictions.

An unknown build or malformed configuration applies zero patches. A failure in one restriction does not convert another restriction's status to success. The final summary makes partial support visible.

Logs must not include passwords, save contents, user tokens, or the contents of proprietary reference artifacts.

## Testing strategy

Implementation follows RED/GREEN TDD.

### Unit tests

- Default configuration retains every documented safety-sensitive restriction.
- Valid overrides affect only named restrictions.
- Missing configuration selects compiled defaults.
- Malformed JSON and wrong types fail closed.
- Unknown keys are reported without changing known values.
- Pattern parsing handles fixed bytes and wildcards and rejects malformed input.
- Scanner tests cover unique, missing, ambiguous, and multiple-expected-match cases.
- Per-restriction planning proves that one failed site produces no writes for that restriction.
- Independent restrictions can succeed and fail separately.
- Original, already-patched, and unexpected-byte states are distinguished.
- Page ranges are coalesced correctly and protection restoration occurs after writes.
- PE and ELF build-identity parsers use redistributable synthetic fixtures.

### Offline proprietary-binary tests

Tests accept local paths through environment variables and skip when the non-redistributable binaries are unavailable. With the pinned files present they must:

- identify the exact Windows and Linux profiles;
- locate every supported patch site with its exact expected count;
- validate every original byte;
- generate a dry-run patch plan without modifying the files;
- reject a modified-byte fixture or mismatched build identity.

No proprietary binary, JMAP, CT table, or extracted reference resource is committed.

### Build and package tests

- Native Linux CMake builds `main.so` with no unresolved symbols.
- xwin/Windows CMake builds `main.dll` in the same UE4SS configuration.
- Export inspection finds exactly the required lifecycle exports plus ordinary runtime dependencies.
- A staged package contains `enabled.txt`, documented `config.json`, `dlls/main.dll`, and `dlls/main.so`.
- The Linux UE4SS loader discovers and runs the mod lifecycle.

### Live Linux acceptance

The updated repository-local server is used on test ports and with the imported `palworld10` save/settings. The unrelated `/palworld` server is never signalled or modified. Acceptance requires:

1. UE4SS starts with all required updated baseline signatures resolved.
2. The mod selects the Linux profile and reports the expected configured restriction results.
3. Live process-memory inspection confirms every reported patch and confirms disabled/default-safe restrictions remain original.
4. The server reaches `Running Palworld dedicated server on :<port>` and accepts a client connection.
5. The world saves after startup and the save remains readable.
6. A 30-minute monitor records bounded RSS and no crash or unexpected exit.
7. A deliberately mismatched profile produces zero writes while the server still reaches ready.

### Windows client acceptance

Automated Linux-hosted evidence includes the Windows DLL build, export/dependency inspection, exact PE profile selection, and a complete dry-run patch plan against the supplied updated client executable. Final runtime acceptance requires the user's Windows machine:

1. Install the packaged mod under the matching UE4SS build.
2. Start Palworld and confirm the profile/applied summary in `UE4SS.log`.
3. Join the updated Linux test server.
4. Exercise representative relaxed placements and representative protected defaults.
5. Restart after changing one toggle and confirm only that behavior changes.
6. Confirm an unmodded or disabled client still joins without crashing.

This manual client placement check occurs after every automated and Linux-side task is complete, as requested by the user. The goal remains unverified until its result is recorded.

## Documentation and provenance

The implementation adds:

- a mod README covering supported targets, exact pinned builds, build commands, installation, configuration semantics, conservative defaults, restart requirements, and Windows-server deferral;
- a restriction/default/support table for Windows client and Linux server;
- troubleshooting for unknown builds, ambiguous patterns, ABI mismatches, and loader failures;
- a Linux live-verification report and a Windows manual-test checklist;
- provenance notes recording reference archive hashes without committing or redistributing their contents.

Known behavioral risks are documented: relaxed support/collision checks may create structures the game later destroys, resource nodes may conflict with structures when their protection is explicitly disabled, and unsafe ownership/limit toggles can damage multiplayer expectations or server performance.

## Completion criteria

Phase 1 is complete only when:

- the design and implementation plan are committed;
- shared core, Windows backend, Linux backend, both target profiles, configuration, packaging, and documentation are implemented;
- all portable, offline target, build, and package tests pass;
- the updated Linux baseline signature suite passes without skips when pointed at the local server;
- live Linux normal and mismatched-profile acceptance passes, including save and 30-minute monitoring;
- the Windows DLL is built and validated offline;
- the user completes and reports the final Windows-client placement/join checklist;
- no Windows-server support is claimed.
