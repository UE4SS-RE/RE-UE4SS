# Palworld Building Restrictions Mod Core Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build and test the cross-platform, restart-required UE4SS mod framework through safe configuration, scanning, patch planning/application, platform memory backends, and fail-closed mod loading, while refreshing the updated Linux Palworld baseline.

**Architecture:** A platform-neutral `BuildingRestrictionsCore` owns the restriction catalog, JSON validation, byte-pattern scanner, target identity, profile selection, and transactional patch engine. A thin UE4SS shared-library target supplies process/module access and logging through Windows and Linux backends; phase-specific game signatures are added only after this core plan passes.

**Tech Stack:** C++23, CMake/CTest, Glaze JSON, UE4SS `CppUserModBase`, Win32 PE APIs, Linux `dl_iterate_phdr`/`mprotect`, x86-64 PE/ELF metadata, Bash acceptance scripts.

---

## File structure

This plan creates the following focused units:

- `cppmods/BuildingRestrictionsDisabler/CMakeLists.txt`: core, mod, and package targets.
- `cppmods/BuildingRestrictionsDisabler/include/BuildingRestrictions/Catalog.hpp`: restriction identifiers and conservative defaults.
- `cppmods/BuildingRestrictionsDisabler/src/Catalog.cpp`: immutable restriction catalog.
- `cppmods/BuildingRestrictionsDisabler/include/BuildingRestrictions/Config.hpp`: validated startup configuration API.
- `cppmods/BuildingRestrictionsDisabler/src/Config.cpp`: Glaze generic-JSON validation and file loading.
- `cppmods/BuildingRestrictionsDisabler/include/BuildingRestrictions/Pattern.hpp`: pattern representation and scanner API.
- `cppmods/BuildingRestrictionsDisabler/src/Pattern.cpp`: strict pattern parser and region scanner.
- `cppmods/BuildingRestrictionsDisabler/include/BuildingRestrictions/Profile.hpp`: target identity, patch sites, and profile registry contracts.
- `cppmods/BuildingRestrictionsDisabler/src/Profile.cpp`: deterministic file identity and exact profile selection.
- `cppmods/BuildingRestrictionsDisabler/include/BuildingRestrictions/PatchEngine.hpp`: dry-run planning and transactional write contracts.
- `cppmods/BuildingRestrictionsDisabler/src/PatchEngine.cpp`: match-count/original-byte validation and per-restriction transactions.
- `cppmods/BuildingRestrictionsDisabler/include/BuildingRestrictions/Platform.hpp`: narrow runtime memory/process interface.
- `cppmods/BuildingRestrictionsDisabler/src/Platform/Linux.cpp`: ELF executable regions, `mprotect`, and cache flushing.
- `cppmods/BuildingRestrictionsDisabler/src/Platform/Win32.cpp`: PE executable regions, `VirtualProtect`, and cache flushing.
- `cppmods/BuildingRestrictionsDisabler/src/Mod.cpp`: UE4SS lifecycle, config location, fail-closed startup, and logging.
- `cppmods/BuildingRestrictionsDisabler/config.json`: shipped conservative configuration.
- `cppmods/BuildingRestrictionsDisabler/enabled.txt`: standard UE4SS enable marker.
- `tests/BuildingRestrictionsCatalogTests.cpp`: catalog/default invariants.
- `tests/BuildingRestrictionsConfigTests.cpp`: JSON success/failure behavior.
- `tests/BuildingRestrictionsPatternTests.cpp`: parser and scanner behavior.
- `tests/BuildingRestrictionsPatchEngineTests.cpp`: transactional patch behavior with a fake backend.
- `tests/BuildingRestrictionsProfileTests.cpp`: deterministic identity and exact selection.
- `tests/BuildingRestrictionsModLoaderTests.cpp`: staged native-mod lifecycle and fail-closed loading.
- `tests/RunBuildingRestrictionsModLoaderTests.cmake`: isolated loader stage.

Game-specific Windows and Linux patch profiles are deliberately not created in this plan. The next plan will derive them with the tested scanner and dry-run tooling, avoiding unreviewable signature code mixed with foundational behavior.

### Task 1: Refresh the updated Linux Palworld baseline

**Files:**
- Modify: `tests/RunPalworldSignatureTests.sh`
- Verify: `tests/PalworldSignatureTests.cpp`

- [ ] **Step 1: Reproduce the pinned-build failure**

Run:

```bash
PALWORLD_SERVER_ROOT=/home/liu/RE-UE4SS/build_linux/palworld-server \
  ctest --test-dir build_linux --output-on-failure -R '^PalworldSignatureTests$'
```

Expected: FAIL with `Palworld executable size mismatch: expected 196281496, got 196285592`.

- [ ] **Step 2: Update only the exact build identity**

Change the constants in `tests/RunPalworldSignatureTests.sh` to:

```bash
expected_size="196285592"
expected_sha256="788649fa1592160faa7bcf07ccd16d474ebeaae954717bc32284b5a43028d8e7"
expected_build_id="7f7e167407984ec3"
```

Change the success line to:

```bash
echo "Palworld Steam build 24181105: size=${actual_size} sha256=${actual_sha256} build_id=${actual_build_id}"
```

Do not make `FUObjectHashTablesGet` required. The current test already reports it as optional/inlined while requiring the nine startup resolvers the runtime actually needs.

- [ ] **Step 3: Verify the refreshed baseline**

Run:

```bash
PALWORLD_SERVER_ROOT=/home/liu/RE-UE4SS/build_linux/palworld-server \
  ctest --test-dir build_linux --output-on-failure -R '^PalworldSignatureTests$'
```

Expected: PASS, prints Steam build `24181105`, resolves every required address, and explicitly reports `FUObjectHashTablesGet` as optional if it remains zero.

- [ ] **Step 4: Commit the baseline refresh**

```bash
git add tests/RunPalworldSignatureTests.sh
git commit -m "test(palworld): pin updated Linux server build"
```

### Task 2: Add the restriction catalog and optional build target

**Files:**
- Modify: `CMakeLists.txt`
- Create: `cppmods/BuildingRestrictionsDisabler/CMakeLists.txt`
- Create: `cppmods/BuildingRestrictionsDisabler/include/BuildingRestrictions/Catalog.hpp`
- Create: `cppmods/BuildingRestrictionsDisabler/src/Catalog.cpp`
- Create: `tests/BuildingRestrictionsCatalogTests.cpp`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write the failing catalog test**

Create `tests/BuildingRestrictionsCatalogTests.cpp`:

```cpp
#include <BuildingRestrictions/Catalog.hpp>

#include <cstdio>
#include <stdexcept>
#include <string_view>

namespace BR = RC::BuildingRestrictions;

namespace
{
    auto expect(bool condition, std::string_view message) -> void
    {
        if (!condition)
        {
            throw std::runtime_error{message.data()};
        }
    }
}

int main()
{
    try
    {
        const auto catalog = BR::restriction_catalog();
        expect(catalog.size() == BR::restriction_count, "catalog size mismatch");

        for (size_t index = 0; index < catalog.size(); ++index)
        {
            expect(static_cast<size_t>(catalog[index].id) == index, "catalog order mismatch");
            expect(!catalog[index].key.empty(), "restriction key was empty");
            expect(!catalog[index].description.empty(), "restriction description was empty");
            for (size_t other = index + 1; other < catalog.size(); ++other)
            {
                expect(catalog[index].key != catalog[other].key, "duplicate restriction key");
            }
        }

        expect(!BR::definition(BR::RestrictionId::OverlapsAnotherObject).default_relaxed,
               "overlap protection must remain enabled by default");
        expect(!BR::definition(BR::RestrictionId::OnOilResource).default_relaxed,
               "oil resource protection must remain enabled by default");
        expect(!BR::definition(BR::RestrictionId::MaxStructuresPerBase).default_relaxed,
               "structure limit must remain enabled by default");
        expect(!BR::definition(BR::RestrictionId::MaxBases).default_relaxed,
               "base limit must remain enabled by default");
        expect(!BR::definition(BR::RestrictionId::OtherGuildBase).default_relaxed,
               "other-guild protection must remain enabled by default");
        expect(BR::definition(BR::RestrictionId::GroundSlope).default_relaxed,
               "ground slope should be relaxed by default");
        expect(BR::definition(BR::RestrictionId::EnoughSupport).default_relaxed,
               "support check should be relaxed by default");
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "BuildingRestrictionsCatalogTests: %s\n", error.what());
        return 1;
    }
    return 0;
}
```

Register the future target in `tests/CMakeLists.txt`:

```cmake
add_executable(BuildingRestrictionsCatalogTests BuildingRestrictionsCatalogTests.cpp)
target_link_libraries(BuildingRestrictionsCatalogTests PRIVATE BuildingRestrictionsCore)
add_test(NAME BuildingRestrictionsCatalogTests COMMAND BuildingRestrictionsCatalogTests)
```

- [ ] **Step 2: Run the test to verify it fails**

Run:

```bash
cmake -S . -B build_linux \
  -DUE4SS_BUILD_TESTS=ON \
  -DUE4SS_BUILD_PALWORLD_BUILDING_MOD=ON \
  -DCMAKE_BUILD_TYPE=Game__Shipping__Linux
cmake --build build_linux --target BuildingRestrictionsCatalogTests --parallel 2
```

Expected: configuration or compilation FAILS because `BuildingRestrictionsCore` and `Catalog.hpp` do not exist.

- [ ] **Step 3: Add the optional subproject and catalog API**

Add before the existing Windows-only `cppmods` block in the root `CMakeLists.txt`:

```cmake
option(UE4SS_BUILD_PALWORLD_BUILDING_MOD "Build the cross-platform Palworld building restrictions mod" OFF)
add_subdirectory("cppmods/BuildingRestrictionsDisabler" EXCLUDE_FROM_ALL)
```

Create `cppmods/BuildingRestrictionsDisabler/CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.22)

add_library(BuildingRestrictionsCore STATIC
    src/Catalog.cpp
)
target_compile_features(BuildingRestrictionsCore PUBLIC cxx_std_23)
target_include_directories(BuildingRestrictionsCore PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
set_target_properties(BuildingRestrictionsCore PROPERTIES POSITION_INDEPENDENT_CODE ON)
```

Create `Catalog.hpp` with this complete public model:

```cpp
#pragma once

#include <array>
#include <cstddef>
#include <span>
#include <string_view>

namespace RC::BuildingRestrictions
{
    enum class RestrictionId : size_t
    {
        OverlapsAnotherObject,
        NotPlacedOnGround,
        AllFloorsTouchGround,
        TooCloseAnotherBase,
        TooCloseBossOrFacility,
        BelowSeaLevel,
        OnOilResource,
        HighLocation,
        MaxStructuresPerBase,
        InDungeon,
        GroundSlope,
        MaxBases,
        WithinBaseRange,
        ObstacleInWay,
        Indoors,
        OtherGuildBase,
        ConnectedToStructure,
        EnoughSupport,
        PlaceWall,
        PlaceCeiling,
        PreventDestroyAfterPlacement,
        DisableSnapping,
        SteepAngleRotation,
        Count,
    };

    inline constexpr size_t restriction_count = static_cast<size_t>(RestrictionId::Count);

    struct RestrictionDefinition
    {
        RestrictionId id{};
        std::string_view key{};
        std::string_view description{};
        bool default_relaxed{};
    };

    auto restriction_catalog() -> std::span<const RestrictionDefinition>;
    auto definition(RestrictionId id) -> const RestrictionDefinition&;
    auto find_restriction(std::string_view key) -> const RestrictionDefinition*;
}
```

Implement `Catalog.cpp` with a `constexpr std::array<RestrictionDefinition, restriction_count>` in exact enum order. Use the JSON keys from the approved spec. Set defaults to `false` for `overlaps_another_object`, `too_close_another_base`, `too_close_boss_or_facility`, `on_oil_resource`, `max_structures_per_base`, `max_bases`, `other_guild_base`, `prevent_destroy_after_placement`, `disable_snapping`, and `steep_angle_rotation`; set every other entry to `true`.

- [ ] **Step 4: Run the focused test**

```bash
cmake -S . -B build_linux \
  -DUE4SS_BUILD_TESTS=ON \
  -DUE4SS_BUILD_PALWORLD_BUILDING_MOD=ON \
  -DCMAKE_BUILD_TYPE=Game__Shipping__Linux
cmake --build build_linux --target BuildingRestrictionsCatalogTests --parallel 2
ctest --test-dir build_linux --output-on-failure -R '^BuildingRestrictionsCatalogTests$'
```

Expected: PASS.

- [ ] **Step 5: Commit the catalog**

```bash
git add CMakeLists.txt cppmods/BuildingRestrictionsDisabler tests/CMakeLists.txt tests/BuildingRestrictionsCatalogTests.cpp
git commit -m "feat(palworld): add building restriction catalog"
```

### Task 3: Implement strict restart-time JSON configuration

**Files:**
- Create: `cppmods/BuildingRestrictionsDisabler/include/BuildingRestrictions/Config.hpp`
- Create: `cppmods/BuildingRestrictionsDisabler/src/Config.cpp`
- Modify: `cppmods/BuildingRestrictionsDisabler/CMakeLists.txt`
- Create: `tests/BuildingRestrictionsConfigTests.cpp`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write the failing configuration tests**

Create `Config.hpp` declarations first so the test expresses the stable interface:

```cpp
#pragma once

#include <BuildingRestrictions/Catalog.hpp>

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace RC::BuildingRestrictions
{
    struct Settings
    {
        bool enabled{true};
        std::array<bool, restriction_count> relaxed{};

        auto is_relaxed(RestrictionId id) const -> bool
        {
            return relaxed[static_cast<size_t>(id)];
        }
    };

    enum class ConfigState
    {
        Loaded,
        MissingDefaults,
        Invalid,
    };

    struct ConfigResult
    {
        Settings settings{};
        ConfigState state{ConfigState::Invalid};
        std::vector<std::string> diagnostics{};

        auto may_patch() const -> bool { return state != ConfigState::Invalid && settings.enabled; }
    };

    auto default_settings() -> Settings;
    auto parse_config(std::optional<std::string_view> source) -> ConfigResult;
    auto load_config(const std::filesystem::path& path) -> ConfigResult;
}
```

Create `tests/BuildingRestrictionsConfigTests.cpp` with these cases:

```cpp
#include <BuildingRestrictions/Config.hpp>

#include <cstdio>
#include <stdexcept>
#include <string_view>

namespace BR = RC::BuildingRestrictions;

namespace
{
    auto expect(bool condition, std::string_view message) -> void
    {
        if (!condition) throw std::runtime_error{message.data()};
    }
}

int main()
{
    try
    {
        const auto missing = BR::parse_config(std::nullopt);
        expect(missing.state == BR::ConfigState::MissingDefaults, "missing config state mismatch");
        expect(missing.may_patch(), "missing config should use enabled conservative defaults");
        expect(!missing.settings.is_relaxed(BR::RestrictionId::OnOilResource), "oil default changed");
        expect(missing.settings.is_relaxed(BR::RestrictionId::GroundSlope), "slope default changed");

        const auto disabled = BR::parse_config(R"({"enabled":false,"restrictions":{"ground_slope":true}})");
        expect(disabled.state == BR::ConfigState::Loaded, "valid disabled config was rejected");
        expect(!disabled.may_patch(), "master disabled config may patch");

        const auto override = BR::parse_config(
                R"({"_comment":"restart required","enabled":true,"restrictions":{"ground_slope":false,"max_bases":true,"future_key":true}})");
        expect(override.state == BR::ConfigState::Loaded, "valid override was rejected");
        expect(!override.settings.is_relaxed(BR::RestrictionId::GroundSlope), "known false override ignored");
        expect(override.settings.is_relaxed(BR::RestrictionId::MaxBases), "known true override ignored");
        expect(!override.diagnostics.empty(), "unknown key was not reported");

        for (const auto invalid_source : {
                     std::string_view{"{"},
                     std::string_view{R"([])"},
                     std::string_view{R"({"enabled":"yes"})"},
                     std::string_view{R"({"restrictions":[]})"},
                     std::string_view{R"({"restrictions":{"ground_slope":1}})"},
             })
        {
            const auto invalid = BR::parse_config(invalid_source);
            expect(invalid.state == BR::ConfigState::Invalid, "invalid config was accepted");
            expect(!invalid.may_patch(), "invalid config may patch");
        }
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "BuildingRestrictionsConfigTests: %s\n", error.what());
        return 1;
    }
    return 0;
}
```

Register and link the test to `BuildingRestrictionsCore`.

- [ ] **Step 2: Run the test to verify it fails**

```bash
cmake --build build_linux --target BuildingRestrictionsConfigTests --parallel 2
```

Expected: FAIL because `Config.cpp` is absent and the symbols are undefined.

- [ ] **Step 3: Implement generic-JSON validation**

Add `src/Config.cpp` to `BuildingRestrictionsCore` and link `glaze::glaze` publicly. Implement `default_settings()` by copying every `default_relaxed` value from the catalog.

Implement `parse_config()` with `glz::read_json<glz::generic>()`. Require the root to be `glz::generic::object_t`; accept `enabled`, `restrictions`, and an optional string `_comment` as root keys; report other root keys. Require `enabled` to be boolean. Require `restrictions` to be an object whose values are boolean. For each restriction key, call `find_restriction`; apply known values and append `unknown restriction key: <key>` for unknown values. On any syntax or type error, return `ConfigState::Invalid`, `Settings{.enabled = false}`, and at least one diagnostic.

Implement `load_config()` with `std::ifstream`: return `parse_config(std::nullopt)` for a missing path, return invalid for other open/read errors, and otherwise pass the complete file text to `parse_config()`.

- [ ] **Step 4: Run focused tests**

```bash
cmake --build build_linux --target BuildingRestrictionsConfigTests --parallel 2
ctest --test-dir build_linux --output-on-failure \
  -R '^(BuildingRestrictionsCatalogTests|BuildingRestrictionsConfigTests)$'
```

Expected: both PASS.

- [ ] **Step 5: Commit configuration support**

```bash
git add cppmods/BuildingRestrictionsDisabler tests/BuildingRestrictionsConfigTests.cpp tests/CMakeLists.txt
git commit -m "feat(palworld): validate building mod configuration"
```

### Task 4: Add strict byte-pattern scanning

**Files:**
- Create: `cppmods/BuildingRestrictionsDisabler/include/BuildingRestrictions/Pattern.hpp`
- Create: `cppmods/BuildingRestrictionsDisabler/src/Pattern.cpp`
- Modify: `cppmods/BuildingRestrictionsDisabler/CMakeLists.txt`
- Create: `tests/BuildingRestrictionsPatternTests.cpp`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Define the scanner interface and failing tests**

Create `Pattern.hpp`:

```cpp
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace RC::BuildingRestrictions
{
    struct Pattern
    {
        std::vector<std::optional<uint8_t>> bytes{};
    };

    struct ScanRegion
    {
        uintptr_t address{};
        std::span<const uint8_t> bytes{};
    };

    auto parse_pattern(std::string_view source) -> std::optional<Pattern>;
    auto scan_pattern(std::span<const ScanRegion> regions, const Pattern& pattern) -> std::vector<uintptr_t>;
}
```

Create tests proving:

```cpp
const auto fixed = BR::parse_pattern("AA 10 ff");
expect(fixed && fixed->bytes.size() == 3, "fixed pattern rejected");
expect(!BR::parse_pattern("A"), "short hex token accepted");
expect(!BR::parse_pattern("GG"), "non-hex token accepted");
expect(!BR::parse_pattern(""), "empty pattern accepted");

std::array<uint8_t, 10> first{0xAA, 0x10, 0xBB, 0x00, 0xAA, 0x20, 0xBB, 0xAA, 0x10, 0xBB};
std::array regions{BR::ScanRegion{0x1000, first}};
const auto wildcard = BR::parse_pattern("AA ?? BB").value();
const auto matches = BR::scan_pattern(regions, wildcard);
expect(matches == std::vector<uintptr_t>({0x1000, 0x1004, 0x1007}), "wildcard scan mismatch");
```

Also cover a pattern longer than a region, adjacent regions that must not be treated as contiguous, and duplicate matches across two regions.

- [ ] **Step 2: Run the test to verify it fails**

```bash
cmake --build build_linux --target BuildingRestrictionsPatternTests --parallel 2
```

Expected: FAIL with undefined pattern functions.

- [ ] **Step 3: Implement the strict parser and scanner**

Parse whitespace-separated tokens only. Accept exactly two hexadecimal digits or `??`. Normalize neither malformed one-character wildcards nor nibble wildcards. Scan each region independently with a straightforward bounded comparison; a wildcard matches any byte. Return absolute addresses in region order.

- [ ] **Step 4: Run focused tests**

```bash
cmake --build build_linux --target BuildingRestrictionsPatternTests --parallel 2
ctest --test-dir build_linux --output-on-failure -R '^BuildingRestrictionsPatternTests$'
```

Expected: PASS.

- [ ] **Step 5: Commit scanning support**

```bash
git add cppmods/BuildingRestrictionsDisabler tests/BuildingRestrictionsPatternTests.cpp tests/CMakeLists.txt
git commit -m "feat(palworld): add strict executable pattern scanner"
```

### Task 5: Add deterministic target identity and profile selection

**Files:**
- Create: `cppmods/BuildingRestrictionsDisabler/include/BuildingRestrictions/Profile.hpp`
- Create: `cppmods/BuildingRestrictionsDisabler/src/Profile.cpp`
- Modify: `cppmods/BuildingRestrictionsDisabler/CMakeLists.txt`
- Create: `tests/BuildingRestrictionsProfileTests.cpp`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Define identity/profile types and failing tests**

Create `Profile.hpp`:

```cpp
#pragma once

#include <BuildingRestrictions/Catalog.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace RC::BuildingRestrictions
{
    enum class ExecutableFormat { PE64, ELF64 };
    enum class TargetRole { WindowsClient, LinuxServer, WindowsServer };

    struct BuildIdentity
    {
        ExecutableFormat format{};
        uint16_t machine{};
        uint64_t file_size{};
        uint64_t file_fingerprint{};
        uint64_t image_size{};
        uint64_t platform_build_id{};

        auto operator==(const BuildIdentity&) const -> bool = default;
    };

    struct PatchSiteDefinition;

    struct RestrictionPatchDefinition
    {
        RestrictionId id{};
        std::span<const PatchSiteDefinition> sites{};
    };

    struct TargetProfile
    {
        std::string_view name{};
        TargetRole role{};
        BuildIdentity identity{};
        std::span<const RestrictionPatchDefinition> restrictions{};
    };

    auto fnv1a64(std::span<const uint8_t> bytes) -> uint64_t;
    auto identify_file(const std::filesystem::path& path) -> std::optional<BuildIdentity>;
    auto select_profile(const BuildIdentity& identity,
                        TargetRole role,
                        std::span<const TargetProfile> profiles) -> const TargetProfile*;
}
```

The test creates minimal redistributable byte fixtures rather than real game files:

- a PE64 fixture with DOS header, PE signature, machine `0x8664`, timestamp `0x12345678`, `SizeOfImage=0x5000`, and one executable section;
- an ELF64 little-endian fixture with machine `62`, one executable `PT_LOAD`, and one GNU build-ID note containing eight known bytes;
- two files that differ by one byte and therefore have different FNV-1a fingerprints;
- two profiles differing only by role, proving identity alone cannot select a Windows server profile for a Windows client.

- [ ] **Step 2: Run the test to verify it fails**

```bash
cmake --build build_linux --target BuildingRestrictionsProfileTests --parallel 2
```

Expected: FAIL because identity parsing and selection are absent.

- [ ] **Step 3: Implement exact file identity**

Implement 64-bit FNV-1a with offset basis `14695981039346656037` and prime `1099511628211` across the complete file bytes.

Implement bounded little-endian readers rather than casting unaligned file buffers. For PE64, validate `MZ`, `PE\0\0`, machine `0x8664`, optional-header magic `0x20b`, and read `TimeDateStamp` into `platform_build_id` plus `SizeOfImage`. For ELF64, validate `0x7fELF`, 64-bit class, little-endian data, machine `62`, compute image size from `PT_LOAD` ranges, and read the GNU build-ID note into an eight-byte little-endian `platform_build_id`. Return `std::nullopt` for truncation, overflow, wrong architecture, or malformed offsets.

`select_profile()` returns a profile only when both role and the complete `BuildIdentity` compare equal. Multiple exact profiles are a configuration error and return `nullptr`.

- [ ] **Step 4: Run focused tests**

```bash
cmake --build build_linux --target BuildingRestrictionsProfileTests --parallel 2
ctest --test-dir build_linux --output-on-failure -R '^BuildingRestrictionsProfileTests$'
```

Expected: PASS.

- [ ] **Step 5: Commit identity support**

```bash
git add cppmods/BuildingRestrictionsDisabler tests/BuildingRestrictionsProfileTests.cpp tests/CMakeLists.txt
git commit -m "feat(palworld): identify exact mod target builds"
```

### Task 6: Implement per-restriction planning and transactional writes

**Files:**
- Modify: `cppmods/BuildingRestrictionsDisabler/include/BuildingRestrictions/Profile.hpp`
- Create: `cppmods/BuildingRestrictionsDisabler/include/BuildingRestrictions/PatchEngine.hpp`
- Create: `cppmods/BuildingRestrictionsDisabler/src/PatchEngine.cpp`
- Modify: `cppmods/BuildingRestrictionsDisabler/CMakeLists.txt`
- Create: `tests/BuildingRestrictionsPatchEngineTests.cpp`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Complete patch-definition types**

Replace the `PatchSiteDefinition` forward declaration with:

```cpp
struct PatchSiteDefinition
{
    std::string_view name{};
    std::string_view pattern{};
    size_t expected_matches{};
    ptrdiff_t patch_offset{};
    std::span<const uint8_t> expected_original{};
    std::span<const uint8_t> replacement{};
};
```

Create `PatchEngine.hpp`:

```cpp
#pragma once

#include <BuildingRestrictions/Pattern.hpp>
#include <BuildingRestrictions/Profile.hpp>

#include <span>
#include <string>
#include <vector>

namespace RC::BuildingRestrictions
{
    enum class PatchStatus
    {
        Applied,
        AlreadyApplied,
        Disabled,
        Unsupported,
        InvalidPattern,
        MatchCountMismatch,
        UnexpectedBytes,
        ProtectionFailure,
        WriteFailure,
        RestoreFailure,
        FlushFailure,
    };

    struct PlannedWrite
    {
        uintptr_t address{};
        std::vector<uint8_t> expected_original{};
        std::vector<uint8_t> replacement{};
    };

    struct PatchResult
    {
        RestrictionId id{};
        PatchStatus status{PatchStatus::Unsupported};
        std::vector<PlannedWrite> writes{};
        std::string diagnostic{};
    };

    class MemoryWriter
    {
      public:
        virtual ~MemoryWriter() = default;
        virtual auto acquire(std::span<const PlannedWrite> writes) -> bool = 0;
        virtual auto write(uintptr_t address, std::span<const uint8_t> bytes) -> bool = 0;
        virtual auto restore() -> bool = 0;
        virtual auto flush(std::span<const PlannedWrite> writes) -> bool = 0;
    };

    auto plan_restriction(const RestrictionPatchDefinition& restriction,
                          std::span<const ScanRegion> regions) -> PatchResult;
    auto apply_plan(PatchResult plan, MemoryWriter& writer) -> PatchResult;
}
```

- [ ] **Step 2: Write failing transactional tests**

Use a vector-backed `FakeWriter` that records calls and can fail acquisition, a selected write, restore, or flush. Cover:

1. one unique original match produces one planned write and applies it;
2. one site expecting two matches plans two writes;
3. a zero or extra match produces `MatchCountMismatch` and no writes;
4. two sites in one restriction where the second has unexpected bytes produce `UnexpectedBytes` and no writes;
5. every match already contains replacement bytes produces `AlreadyApplied`;
6. a mix of original and already-patched matches is rejected as `UnexpectedBytes` to avoid a partially known state;
7. acquisition failure performs zero writes;
8. write failure calls restore and stops;
9. restore or flush failure returns its exact fatal status.

- [ ] **Step 3: Run the test to verify it fails**

```bash
cmake --build build_linux --target BuildingRestrictionsPatchEngineTests --parallel 2
```

Expected: FAIL because planner/application functions are absent.

- [ ] **Step 4: Implement planning and application**

`plan_restriction()` parses every site pattern, scans executable regions, requires exact match counts, bounds-checks `patch_offset`, and compares bytes at every target. It returns no writes unless all sites validate. Sort writes by address and reject overlapping writes unless their expected and replacement bytes are identical.

`apply_plan()` calls `acquire()` once before the first write. It writes in ascending address order, always calls `restore()` after a successful acquire, and calls `flush()` only after all writes and restoration succeed. Any failure prevents subsequent operations except the mandatory restoration attempt.

- [ ] **Step 5: Run focused tests**

```bash
cmake --build build_linux --target BuildingRestrictionsPatchEngineTests --parallel 2
ctest --test-dir build_linux --output-on-failure \
  -R '^(BuildingRestrictionsPatternTests|BuildingRestrictionsPatchEngineTests)$'
```

Expected: both PASS.

- [ ] **Step 6: Commit patch-engine support**

```bash
git add cppmods/BuildingRestrictionsDisabler tests/BuildingRestrictionsPatchEngineTests.cpp tests/CMakeLists.txt
git commit -m "feat(palworld): apply restriction patches transactionally"
```

### Task 7: Add Windows and Linux runtime memory backends

**Files:**
- Create: `cppmods/BuildingRestrictionsDisabler/include/BuildingRestrictions/Platform.hpp`
- Create: `cppmods/BuildingRestrictionsDisabler/src/Platform/Linux.cpp`
- Create: `cppmods/BuildingRestrictionsDisabler/src/Platform/Win32.cpp`
- Modify: `cppmods/BuildingRestrictionsDisabler/CMakeLists.txt`
- Create: `tests/BuildingRestrictionsPlatformTests.cpp`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Define the runtime interface**

Create `Platform.hpp`:

```cpp
#pragma once

#include <BuildingRestrictions/PatchEngine.hpp>
#include <BuildingRestrictions/Profile.hpp>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace RC::BuildingRestrictions
{
    struct ProcessImage
    {
        std::filesystem::path executable_path{};
        std::string executable_name{};
        TargetRole role{};
        BuildIdentity identity{};
        std::vector<ScanRegion> executable_regions{};
    };

    class RuntimePlatform : public MemoryWriter
    {
      public:
        virtual auto process_image() -> std::optional<ProcessImage> = 0;
    };

    auto make_runtime_platform() -> std::unique_ptr<RuntimePlatform>;
}
```

- [ ] **Step 2: Add a Linux-focused platform test**

`tests/BuildingRestrictionsPlatformTests.cpp` obtains the backend for the test executable and asserts:

- the current executable path exists;
- role detection returns no Palworld role for the test basename, so `process_image()` returns `std::nullopt` without changing memory;
- a test-only helper exposed from `Platform.hpp` maps `Palworld-Win64-Shipping.exe` to `WindowsClient`, `PalServer-Linux-Shipping` to `LinuxServer`, `PalServer-Win64-Shipping-Cmd.exe` to `WindowsServer`, and rejects other names.

Keep real memory-protection behavior covered by the fake-writer transaction tests; do not make code pages writable in unit tests.

- [ ] **Step 3: Run the test to verify it fails**

```bash
cmake --build build_linux --target BuildingRestrictionsPlatformTests --parallel 2
```

Expected: FAIL because the platform factory and role detection are absent.

- [ ] **Step 4: Implement the Linux backend**

Use `/proc/self/exe` for the executable path and `dl_iterate_phdr` for the main image (`dlpi_name` empty or filesystem-equivalent to `/proc/self/exe`). Add one `ScanRegion` for each `PT_LOAD` segment with `PF_X`. Record each affected page's original `PF_R/PF_W/PF_X` protection before calling `mprotect` with added `PROT_WRITE`. Acquire all pages before writes, restore all pages even after a write failure, and call `__builtin___clear_cache` over the minimum/maximum written address range.

- [ ] **Step 5: Implement the Windows backend**

Use `GetModuleHandleW(nullptr)` and `GetModuleFileNameW` for the main image. Parse PE section headers and add sections with `IMAGE_SCN_MEM_EXECUTE`. Use `VirtualProtect` to acquire every page/range and retain each returned original protection. Restore every range with its exact saved value, and call `FlushInstructionCache(GetCurrentProcess(), ...)` after writes.

Both backends call `identify_file()` on the executable path and reject an identity format that does not match the operating system. They reject the deferred Windows-server role with a normal unsupported-profile result later; role detection itself still recognizes the basename.

- [ ] **Step 6: Run Linux platform and core tests**

```bash
cmake --build build_linux --target BuildingRestrictionsPlatformTests --parallel 2
ctest --test-dir build_linux --output-on-failure \
  -R '^BuildingRestrictions(Catalog|Config|Pattern|Profile|PatchEngine|Platform)Tests$'
```

Expected: all listed tests PASS.

- [ ] **Step 7: Commit platform backends**

```bash
git add cppmods/BuildingRestrictionsDisabler tests/BuildingRestrictionsPlatformTests.cpp tests/CMakeLists.txt
git commit -m "feat(palworld): add cross-platform patch memory backends"
```

### Task 8: Build a fail-closed UE4SS mod and package scaffold

**Files:**
- Create: `cppmods/BuildingRestrictionsDisabler/src/Mod.cpp`
- Create: `cppmods/BuildingRestrictionsDisabler/config.json`
- Create: `cppmods/BuildingRestrictionsDisabler/enabled.txt`
- Modify: `cppmods/BuildingRestrictionsDisabler/CMakeLists.txt`
- Create: `tests/BuildingRestrictionsModLoaderTests.cpp`
- Create: `tests/RunBuildingRestrictionsModLoaderTests.cmake`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Add an empty profile registry API**

Add to `Profile.hpp`:

```cpp
auto target_profiles() -> std::span<const TargetProfile>;
```

For this core plan, implement it in `Profile.cpp` as an empty span. The next plan replaces it with the two pinned profiles.

- [ ] **Step 2: Write the failing loader integration test**

Create a small test executable that constructs `RC::CppMod` against a staged `Mods/BuildingRestrictionsDisabler` directory, starts the mod, fires program/unreal-init callbacks, uninstalls it, and checks a log file selected with `BUILDING_RESTRICTIONS_TEST_LOG` for:

```text
start_mod
config_state=loaded
profile_status=unsupported_process
summary applied=0
uninstall_mod
```

The test stage copies `main.so`, `enabled.txt`, and `config.json`; it sets `UE4SS_DISABLE_AUTO_START=1` and must exit without modifying the test executable.

- [ ] **Step 3: Run the loader test to verify it fails**

```bash
cmake -S . -B build_linux \
  -DUE4SS_BUILD_TESTS=ON \
  -DUE4SS_BUILD_PALWORLD_BUILDING_MOD=ON \
  -DCMAKE_BUILD_TYPE=Game__Shipping__Linux
cmake --build build_linux --target BuildingRestrictionsModLoaderTests --parallel 2
ctest --test-dir build_linux --output-on-failure -R '^BuildingRestrictionsModLoaderTests$'
```

Expected: FAIL because the mod target and stage do not exist.

- [ ] **Step 4: Implement the native mod lifecycle**

`Mod.cpp` defines `BuildingRestrictionsMod final : RC::CppUserModBase`. Its constructor:

1. sets `ModName`, `ModVersion`, `ModDescription`, and `ModAuthors`;
2. records `start_mod` in the test log when the environment variable is set;
3. resolves `<working directory>/Mods/BuildingRestrictionsDisabler/config.json` through `UE4SSProgram::get_program()`;
4. loads configuration and logs every diagnostic;
5. stops with zero writes when the master switch is off, configuration is invalid, the process role is unsupported, build identity is unknown, or the registry is empty;
6. otherwise plans and applies enabled supported restrictions in catalog order and emits a final summary.

Export only:

```cpp
#ifdef _WIN32
#define BR_MOD_EXPORT extern "C" __declspec(dllexport)
#else
#define BR_MOD_EXPORT extern "C" __attribute__((visibility("default")))
#endif

BR_MOD_EXPORT RC::CppUserModBase* start_mod()
{
    return new BuildingRestrictionsMod{};
}

BR_MOD_EXPORT void uninstall_mod(RC::CppUserModBase* mod)
{
    delete mod;
}
```

The destructor records `uninstall_mod` for the integration test and never restores code bytes.

- [ ] **Step 5: Complete CMake and shipped files**

When `UE4SS_BUILD_PALWORLD_BUILDING_MOD=ON`, add `BuildingRestrictionsDisabler` as a shared library from `Mod.cpp` plus the current platform backend, link `BuildingRestrictionsCore` and `UE4SS`, use hidden default visibility on Linux, set `OUTPUT_NAME main`, and put the binary under a predictable `BuildingRestrictionsDisabler/dlls` output directory.

Create `enabled.txt` as an empty file. Create `config.json` with the exact schema/defaults from the approved design spec and a leading `_comment` string explaining that `true` disables the named game restriction and restart is required. Treat `_comment` as a documented ignored root key rather than an unknown-key warning.

- [ ] **Step 6: Run loader and regression tests**

```bash
cmake --build build_linux --target BuildingRestrictionsDisabler BuildingRestrictionsModLoaderTests --parallel 2
ctest --test-dir build_linux --output-on-failure \
  -R '^BuildingRestrictions(Catalog|Config|Pattern|Profile|PatchEngine|Platform|ModLoader)Tests$'
```

Expected: all tests PASS; the mod reports an unsupported process and zero writes in the non-Palworld fixture. Exact unknown-build behavior remains covered by `BuildingRestrictionsProfileTests` until the real profile plan adds a Palworld runtime integration case.

- [ ] **Step 7: Inspect the Linux library**

```bash
ldd -r build_linux/Game__Shipping__Linux/BuildingRestrictionsDisabler/dlls/main.so
nm -D --defined-only build_linux/Game__Shipping__Linux/BuildingRestrictionsDisabler/dlls/main.so \
  | rg ' (start_mod|uninstall_mod)$'
```

Expected: no unresolved symbols; both lifecycle exports are present.

- [ ] **Step 8: Commit the fail-closed mod scaffold**

```bash
git add cppmods/BuildingRestrictionsDisabler tests/BuildingRestrictionsModLoaderTests.cpp \
  tests/RunBuildingRestrictionsModLoaderTests.cmake tests/CMakeLists.txt
git commit -m "feat(palworld): add fail-closed building mod runtime"
```

### Task 9: Verify and document the core milestone

**Files:**
- Create: `cppmods/BuildingRestrictionsDisabler/README.md`
- Create: `docs/superpowers/verification/2026-07-15-palworld-building-mod-core-report.md`
- Modify: `docs/superpowers/plans/2026-07-15-palworld-building-mod-core.md`

- [ ] **Step 1: Run the complete native regression suite**

```bash
cmake -S . -B build_linux \
  -DUE4SS_BUILD_TESTS=ON \
  -DUE4SS_BUILD_PALWORLD_BUILDING_MOD=ON \
  -DCMAKE_BUILD_TYPE=Game__Shipping__Linux
cmake --build build_linux --target UE4SS BuildingRestrictionsDisabler \
  BuildingRestrictionsCatalogTests BuildingRestrictionsConfigTests \
  BuildingRestrictionsPatternTests BuildingRestrictionsProfileTests \
  BuildingRestrictionsPatchEngineTests BuildingRestrictionsPlatformTests \
  BuildingRestrictionsModLoaderTests --parallel 2
ctest --test-dir build_linux --output-on-failure
PALWORLD_SERVER_ROOT=/home/liu/RE-UE4SS/build_linux/palworld-server \
  ctest --test-dir build_linux --output-on-failure -R '^PalworldSignatureTests$'
```

Expected: all registered tests pass; the non-redistributable Palworld test runs without a skip and passes for build `24181105`.

- [ ] **Step 2: Write the core README**

Document:

- phase-1 target matrix and Windows-server deferral;
- why the mod is a clean-room implementation rather than a converted DLL;
- restart-required behavior and lack of hotkeys/overlay;
- configuration semantics and conservative defaults;
- current milestone status: runtime core complete, game patch profiles intentionally pending the next plan;
- CMake commands used above;
- the rule that no reference artifact is committed or redistributed.

- [ ] **Step 3: Write the verification report**

Record exact commit, compiler, commands, results, updated Linux target identity, library dependency/export output, and the expected fail-closed loader summary. State explicitly that no real building restriction is patched until the profile plan lands.

- [ ] **Step 4: Self-review plan completion**

Mark every completed checkbox in this plan. Run:

```bash
rg -n '\- \[ \]' docs/superpowers/plans/2026-07-15-palworld-building-mod-core.md
git diff --check
git status --short
```

Expected: no unchecked step in this plan, no whitespace errors, and only the pre-existing untracked `.xmake/` remains outside intended changes.

- [ ] **Step 5: Commit core documentation**

```bash
git add cppmods/BuildingRestrictionsDisabler/README.md \
  docs/superpowers/verification/2026-07-15-palworld-building-mod-core-report.md \
  docs/superpowers/plans/2026-07-15-palworld-building-mod-core.md
git commit -m "docs(palworld): record building mod core verification"
```

## Next plan entry criteria

Do not begin game signature work until this plan is fully green. The profile plan may then rely on:

- exact configuration/default behavior;
- a strict wildcard scanner;
- deterministic PE/ELF target identity;
- dry-run per-restriction planning;
- transactional runtime writes;
- a loadable Windows/Linux UE4SS mod that already fails closed.
