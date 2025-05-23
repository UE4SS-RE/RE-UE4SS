# Toolchain file for using system clang-cl on Windows
# Expects clang-cl to be installed at C:\Program Files\LLVM\bin\clang-cl.exe

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_VERSION 10.0)

# Set the compilers
set(CMAKE_C_COMPILER "C:/Program Files/LLVM/bin/clang-cl.exe" CACHE FILEPATH "C compiler")
set(CMAKE_CXX_COMPILER "C:/Program Files/LLVM/bin/clang-cl.exe" CACHE FILEPATH "C++ compiler")

# Set the linker
set(CMAKE_LINKER "C:/Program Files/LLVM/bin/lld-link.exe" CACHE FILEPATH "Linker")

# Tell CMake we're using clang-cl
set(CMAKE_C_COMPILER_ID "Clang" CACHE STRING "C compiler ID")
set(CMAKE_CXX_COMPILER_ID "Clang" CACHE STRING "C++ compiler ID")
set(CMAKE_C_COMPILER_FRONTEND_VARIANT "MSVC" CACHE STRING "C compiler frontend")
set(CMAKE_CXX_COMPILER_FRONTEND_VARIANT "MSVC" CACHE STRING "C++ compiler frontend")

# Set the resource compiler (using Windows SDK rc.exe)
set(CMAKE_RC_COMPILER "rc.exe" CACHE FILEPATH "Resource compiler")

# Ensure proper Windows SDK is found
set(CMAKE_WINDOWS_KITS_10_DIR "$ENV{WindowsSdkDir}" CACHE PATH "Windows SDK directory")

# Set compiler flags for clang-cl compatibility
set(CMAKE_C_FLAGS_INIT "/DWIN32 /D_WINDOWS /W3 /GR /EHsc")
set(CMAKE_CXX_FLAGS_INIT "/DWIN32 /D_WINDOWS /W3 /GR /EHsc")

# Debug and Release specific flags
set(CMAKE_C_FLAGS_DEBUG_INIT "/MDd /Zi /Ob0 /Od /RTC1")
set(CMAKE_CXX_FLAGS_DEBUG_INIT "/MDd /Zi /Ob0 /Od /RTC1")
set(CMAKE_C_FLAGS_RELEASE_INIT "/MD /O2 /Ob2 /DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "/MD /O2 /Ob2 /DNDEBUG")

# Set the standard libraries
set(CMAKE_C_STANDARD_LIBRARIES "kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib" CACHE STRING "C standard libraries")
set(CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES}" CACHE STRING "C++ standard libraries")