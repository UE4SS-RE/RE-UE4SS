cmake_minimum_required(VERSION 3.22)

# Native unit tests (assert-style, no framework).
# Enabled with -DUE4SS_BUILD_TESTS=ON; run via `ctest`.

add_executable(FileTests FileTests.cpp)
target_link_libraries(FileTests PRIVATE File Helpers fmt)
target_compile_definitions(FileTests PRIVATE RC_FILE_BUILD_STATIC)
add_test(NAME FileTests COMMAND FileTests)

add_executable(SigScannerTests SigScannerTests.cpp)
target_link_libraries(SigScannerTests PRIVATE SinglePassSigScanner Profiler fmt)
target_compile_definitions(SigScannerTests PRIVATE RC_SINGLE_PASS_SIG_SCANNER_BUILD_STATIC)
add_test(NAME SigScannerTests COMMAND SigScannerTests)

add_executable(HeadlessHeadersTests HeadlessHeadersTests.cpp)
target_include_directories(HeadlessHeadersTests PRIVATE
    "${CMAKE_SOURCE_DIR}/UE4SS/include"
    "${CMAKE_SOURCE_DIR}/UE4SS/generated_include"
)
target_link_libraries(HeadlessHeadersTests PRIVATE
    File glaze::glaze DynamicOutput
    Unreal SinglePassSigScanner LuaMadeSimple
    Function IniParser JSON Input Constructs Helpers
    MProgram ScopedTimer Profiler patternsleuth_bind
    fmt PolyHook_2
)
target_compile_definitions(HeadlessHeadersTests PRIVATE
    RC_UE4SS_EXPORTS
    RC_ASM_HELPER_BUILD_STATIC
    RC_FILE_BUILD_STATIC
    RC_DYNAMIC_OUTPUT_BUILD_STATIC
    RC_SINGLE_PASS_SIG_SCANNER_BUILD_STATIC
    RC_SINGLE_PASS_SIG_SCANNER_STATIC
    RC_UNREAL_BUILD_STATIC
    RC_INPUT_BUILD_STATIC
    RC_LUA_MADE_SIMPLE_BUILD_STATIC
    RC_FUNCTION_TIMER_BUILD_STATIC
    RC_PARSER_BASE_BUILD_STATIC
    RC_INI_PARSER_BUILD_STATIC
    RC_JSON_BUILD_STATIC
    RC_JSON_PARSER_BUILD_STATIC
    RC_LUA_WRAPPER_GENERATOR_BUILD_STATIC
)
add_test(NAME HeadlessHeadersTests COMMAND HeadlessHeadersTests)

add_executable(LuaMadeSimpleIntegralTests LuaMadeSimpleIntegralTests.cpp)
target_link_libraries(LuaMadeSimpleIntegralTests PRIVATE LuaMadeSimple)
target_compile_definitions(LuaMadeSimpleIntegralTests PRIVATE RC_LUA_MADE_SIMPLE_BUILD_STATIC)
add_test(NAME LuaMadeSimpleIntegralTests COMMAND LuaMadeSimpleIntegralTests)

add_executable(UnrealStringFormatTests UnrealStringFormatTests.cpp)
target_link_libraries(UnrealStringFormatTests PRIVATE Unreal Helpers DynamicOutput patternsleuth_bind fmt)
target_compile_definitions(UnrealStringFormatTests PRIVATE
    RC_UNREAL_BUILD_STATIC
    RC_DYNAMIC_OUTPUT_BUILD_STATIC
    RC_FILE_BUILD_STATIC
)
add_test(NAME UnrealStringFormatTests COMMAND UnrealStringFormatTests)

add_executable(PatternSleuthElfFixture fixtures/PatternSleuthElfFixture.cpp)
add_executable(PatternSleuthElfTests PatternSleuthElfTests.cpp)
target_link_libraries(PatternSleuthElfTests PRIVATE patternsleuth_bind)
add_test(NAME PatternSleuthElfTests COMMAND PatternSleuthElfTests "$<TARGET_FILE:PatternSleuthElfFixture>")

add_executable(FilesystemWatcherTests
    FilesystemWatcherTests.cpp
    "${CMAKE_SOURCE_DIR}/UE4SS/src/FilesystemWatcher.cpp"
)
target_include_directories(FilesystemWatcherTests PRIVATE "${CMAKE_SOURCE_DIR}/UE4SS/include")
target_link_libraries(FilesystemWatcherTests PRIVATE DynamicOutput File Helpers fmt)
target_compile_definitions(FilesystemWatcherTests PRIVATE
    RC_DYNAMIC_OUTPUT_BUILD_STATIC
    RC_FILE_BUILD_STATIC
)
add_test(NAME FilesystemWatcherTests COMMAND FilesystemWatcherTests)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(LinuxHookTests LinuxHookTests.cpp)
    target_link_libraries(LinuxHookTests PRIVATE PolyHook_2)
    add_test(NAME LinuxHookTests COMMAND LinuxHookTests)

    add_executable(LinuxFNameHookTests LinuxFNameHookTests.cpp)
    target_link_libraries(LinuxFNameHookTests PRIVATE Unreal patternsleuth_bind)
    add_test(NAME LinuxFNameHookTests COMMAND LinuxFNameHookTests)

    add_executable(LinuxFMallocVTableTests LinuxFMallocVTableTests.cpp)
    target_link_libraries(LinuxFMallocVTableTests PRIVATE Unreal patternsleuth_bind)
    add_test(NAME LinuxFMallocVTableTests COMMAND LinuxFMallocVTableTests)

    add_executable(LinuxFPropertyLayoutTests LinuxFPropertyLayoutTests.cpp)
    target_link_libraries(LinuxFPropertyLayoutTests PRIVATE Unreal patternsleuth_bind)
    add_test(NAME LinuxFPropertyLayoutTests COMMAND LinuxFPropertyLayoutTests)

    add_executable(LinuxUEnumLayoutTests LinuxUEnumLayoutTests.cpp)
    target_link_libraries(LinuxUEnumLayoutTests PRIVATE Unreal patternsleuth_bind)
    add_test(NAME LinuxUEnumLayoutTests COMMAND LinuxUEnumLayoutTests)

    add_executable(LinuxRequiredScanFailSoftTests LinuxRequiredScanFailSoftTests.cpp)
    target_link_libraries(LinuxRequiredScanFailSoftTests PRIVATE Unreal patternsleuth_bind)
    target_link_options(LinuxRequiredScanFailSoftTests PRIVATE "-Wl,--wrap=ps_scan")
    add_test(NAME LinuxRequiredScanFailSoftTests COMMAND LinuxRequiredScanFailSoftTests)

    add_executable(LinuxSDKOutputPathTests LinuxSDKOutputPathTests.cpp)
    target_include_directories(LinuxSDKOutputPathTests PRIVATE "${CMAKE_SOURCE_DIR}/UE4SS/include")
    target_link_libraries(LinuxSDKOutputPathTests PRIVATE Unreal patternsleuth_bind)
    add_test(NAME LinuxSDKOutputPathTests COMMAND LinuxSDKOutputPathTests)

    add_executable(LinuxVirtualFunctionLayoutTests LinuxVirtualFunctionLayoutTests.cpp)
    target_link_libraries(LinuxVirtualFunctionLayoutTests PRIVATE Unreal patternsleuth_bind)
    add_test(NAME LinuxVirtualFunctionLayoutTests COMMAND LinuxVirtualFunctionLayoutTests)

    add_executable(LinuxUEngineVTableTests LinuxUEngineVTableTests.cpp)
    target_link_libraries(LinuxUEngineVTableTests PRIVATE Unreal patternsleuth_bind)
    add_test(NAME LinuxUEngineVTableTests COMMAND LinuxUEngineVTableTests)

    add_executable(LinuxLoadMapOptimizedABIHookTests LinuxLoadMapOptimizedABIHookTests.cpp)
    target_link_libraries(LinuxLoadMapOptimizedABIHookTests PRIVATE Unreal patternsleuth_bind)
    add_test(NAME LinuxLoadMapOptimizedABIHookTests COMMAND LinuxLoadMapOptimizedABIHookTests)

    add_executable(LinuxLoadMapStandardABIHookTests LinuxLoadMapStandardABIHookTests.cpp)
    target_link_libraries(LinuxLoadMapStandardABIHookTests PRIVATE Unreal patternsleuth_bind)
    add_test(NAME LinuxLoadMapStandardABIHookTests COMMAND LinuxLoadMapStandardABIHookTests)

    add_executable(LinuxProcessLocalScriptFunctionResolverTests LinuxProcessLocalScriptFunctionResolverTests.cpp)
    target_link_libraries(LinuxProcessLocalScriptFunctionResolverTests PRIVATE Unreal patternsleuth_bind)
    add_test(NAME LinuxProcessLocalScriptFunctionResolverTests COMMAND LinuxProcessLocalScriptFunctionResolverTests)

    add_executable(CrashDumperProbe
        CrashDumperProbe.cpp
        "${CMAKE_SOURCE_DIR}/UE4SS/src/Platform/Linux/CrashDumper.cpp"
    )
    target_include_directories(CrashDumperProbe PRIVATE "${CMAKE_SOURCE_DIR}/UE4SS/include")
    target_link_libraries(CrashDumperProbe PRIVATE dl)

    add_executable(CrashDumperVerifier CrashDumperVerifier.cpp)

    set(CRASH_DUMPER_TEST_DIR "${CMAKE_CURRENT_BINARY_DIR}/crash-dumper-output")
    add_test(NAME CrashDumperProbe COMMAND CrashDumperProbe "${CRASH_DUMPER_TEST_DIR}")
    set_tests_properties(CrashDumperProbe PROPERTIES WILL_FAIL TRUE)
    add_test(NAME CrashDumperVerifier COMMAND CrashDumperVerifier "${CRASH_DUMPER_TEST_DIR}")
    set_tests_properties(CrashDumperVerifier PROPERTIES DEPENDS CrashDumperProbe)

    add_library(SharedLibraryFixture SHARED fixtures/SharedLibraryFixture.cpp)
    add_executable(SharedLibraryTests
        SharedLibraryTests.cpp
        "${CMAKE_SOURCE_DIR}/UE4SS/src/Platform/Linux/SharedLibrary.cpp"
    )
    target_include_directories(SharedLibraryTests PRIVATE "${CMAKE_SOURCE_DIR}/UE4SS/include")
    target_link_libraries(SharedLibraryTests PRIVATE dl)
    add_test(NAME SharedLibraryTests COMMAND SharedLibraryTests "$<TARGET_FILE:SharedLibraryFixture>")

    add_executable(LinuxPreloadProbe LinuxPreloadProbe.cpp)
    target_link_libraries(LinuxPreloadProbe PRIVATE dl)
    add_test(NAME LinuxPreloadProbe
        COMMAND "${CMAKE_COMMAND}" -E env
            "LD_PRELOAD=$<TARGET_FILE:UE4SS>"
            "UE4SS_DISABLE_AUTO_START=1"
            "$<TARGET_FILE:LinuxPreloadProbe>"
            "$<TARGET_FILE:SharedLibraryFixture>"
    )
    set_tests_properties(LinuxPreloadProbe PROPERTIES PASS_REGULAR_EXPRESSION "UE4SS_PRELOAD_PROBE_READY")

    add_library(CppModProbe SHARED fixtures/CppModProbe.cpp)
    target_link_libraries(CppModProbe PRIVATE UE4SS)

    add_executable(CppModLoaderTests CppModLoaderTests.cpp)
    target_link_libraries(CppModLoaderTests PRIVATE UE4SS)
    add_test(NAME CppModLoaderTests
        COMMAND "${CMAKE_COMMAND}" -E env
            "UE4SS_DISABLE_AUTO_START=1"
            "$<TARGET_FILE:CppModLoaderTests>"
            "$<TARGET_FILE:CppModProbe>"
            "$<TARGET_FILE:UE4SS>"
    )

    add_executable(LinuxLuaModDiscoveryTests LinuxLuaModDiscoveryTests.cpp)
    target_link_libraries(LinuxLuaModDiscoveryTests PRIVATE UE4SS)
    add_test(NAME LinuxLuaModDiscoveryTests
        COMMAND "${CMAKE_COMMAND}" -E env
            "UE4SS_DISABLE_AUTO_START=1"
            "$<TARGET_FILE:LinuxLuaModDiscoveryTests>"
            "${CMAKE_SOURCE_DIR}/assets/UE4SS-settings.ini"
    )

    add_executable(LinuxFailSoftProbe LinuxFailSoftProbe.cpp)
    add_test(NAME LinuxFailSoftProbe
        COMMAND "${CMAKE_COMMAND}"
            "-DUE4SS_LIBRARY=$<TARGET_FILE:UE4SS>"
            "-DPROBE_EXECUTABLE=$<TARGET_FILE:LinuxFailSoftProbe>"
            "-DSETTINGS_TEMPLATE=${CMAKE_SOURCE_DIR}/assets/UE4SS-settings.ini"
            "-DSTAGE_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}/linux-fail-soft-stage"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/RunLinuxFailSoftProbe.cmake"
    )

    add_library(HostilePersonalityFixture SHARED fixtures/HostilePersonalityFixture.cpp)
    add_executable(LinuxPersonalityConflictProbe LinuxPersonalityConflictProbe.cpp)
    target_link_libraries(LinuxPersonalityConflictProbe PRIVATE HostilePersonalityFixture)
    add_test(NAME LinuxPersonalityConflictProbe
        COMMAND "${CMAKE_COMMAND}"
            "-DUE4SS_LIBRARY=$<TARGET_FILE:UE4SS>"
            "-DPROBE_EXECUTABLE=$<TARGET_FILE:LinuxPersonalityConflictProbe>"
            "-DSETTINGS_TEMPLATE=${CMAKE_SOURCE_DIR}/assets/UE4SS-settings.ini"
            "-DSTAGE_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}/linux-personality-conflict-stage"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/RunLinuxFailSoftProbe.cmake"
    )

    add_executable(PalworldSignatureTests PalworldSignatureTests.cpp)
    target_link_libraries(PalworldSignatureTests PRIVATE patternsleuth_bind)
    add_test(NAME PalworldSignatureTests
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/RunPalworldSignatureTests.sh"
            "$<TARGET_FILE:PalworldSignatureTests>"
    )
    set_tests_properties(PalworldSignatureTests PROPERTIES SKIP_RETURN_CODE 77)

    add_test(NAME PalworldAcceptanceHarnessTests
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/palworld/test_run_acceptance.sh"
    )

    add_executable(LinuxLauncherProbe LinuxLauncherProbe.cpp)
    target_link_libraries(LinuxLauncherProbe PRIVATE dl)
    add_test(NAME LinuxLauncherTests
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/RunLinuxLauncherTests.sh"
            "${CMAKE_SOURCE_DIR}/tools/linux/run_ue4ss.sh"
            "$<TARGET_FILE:UE4SS>"
            "$<TARGET_FILE:LinuxLauncherProbe>"
            "$<TARGET_FILE:SharedLibraryFixture>"
            "${CMAKE_CURRENT_BINARY_DIR}/linux launcher stage"
    )

    add_executable(LinuxStartupPolicyTests
        LinuxStartupPolicyTests.cpp
        "${CMAKE_SOURCE_DIR}/UE4SS/src/Platform/Linux/StartupPolicy.cpp"
    )
    target_include_directories(LinuxStartupPolicyTests PRIVATE "${CMAKE_SOURCE_DIR}/UE4SS/include")
    add_test(NAME LinuxStartupPolicyTests COMMAND LinuxStartupPolicyTests)

    add_executable(LinuxPreloadScopeProbe LinuxPreloadScopeProbe.cpp)
    target_link_libraries(LinuxPreloadScopeProbe PRIVATE dl)
    add_test(NAME LinuxPreloadScopeTests
        COMMAND "${CMAKE_COMMAND}"
            "-DUE4SS_LIBRARY=$<TARGET_FILE:UE4SS>"
            "-DLAUNCHER_SOURCE=${CMAKE_SOURCE_DIR}/tools/linux/run_ue4ss.sh"
            "-DPROBE_EXECUTABLE=$<TARGET_FILE:LinuxPreloadScopeProbe>"
            "-DEXISTING_PRELOAD=$<TARGET_FILE:SharedLibraryFixture>"
            "-DSETTINGS_TEMPLATE=${CMAKE_SOURCE_DIR}/assets/UE4SS-settings.ini"
            "-DSTAGE_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}/linux-preload-scope-stage"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/RunLinuxPreloadScopeTests.cmake"
    )
endif()
