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
