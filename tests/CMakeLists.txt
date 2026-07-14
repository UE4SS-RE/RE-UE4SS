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
