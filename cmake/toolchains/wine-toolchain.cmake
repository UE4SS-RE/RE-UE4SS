# Consolidated Wine toolchain file for cross-compiling from Linux to Windows
#
# This file provides common Wine setup for both MSVC and clang-cl compilers.
# It should be included by specific toolchain files that set the WINE_COMPILER variable.
#
# Required variables (set before including this file):
#   WINE_COMPILER - Either "MSVC" or "CLANG_CL"
#
# Prerequisites:
#   - Wine installed and configured
#   - For MSVC: msvc-wine installed (https://github.com/mstorsjo/msvc-wine)
#   - For CLANG_CL: Windows LLVM/Clang installed in Wine or clang-cl wrapper
#
# Optional environment variables:
#   WINE_PREFIX - Path to Wine prefix (default: ~/.wine)
#   MSVC_WINE_CL - Path to cl.exe for MSVC mode
#   CLANG_CL_WINE - Path to clang-cl for CLANG_CL mode

# Validate that WINE_COMPILER is set
if(NOT DEFINED WINE_COMPILER)
    message(FATAL_ERROR "WINE_COMPILER must be set to either MSVC or CLANG_CL before including wine-toolchain.cmake")
endif()

if(NOT WINE_COMPILER STREQUAL "MSVC" AND NOT WINE_COMPILER STREQUAL "CLANG_CL")
    message(FATAL_ERROR "WINE_COMPILER must be either MSVC or CLANG_CL, got: ${WINE_COMPILER}")
endif()

# Common system settings
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Set architecture info for GNUInstallDirs
set(CMAKE_SIZEOF_VOID_P 8)

# Skip compiler checks - Wine makes these unreliable
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)
set(CMAKE_C_ABI_COMPILED 1)
set(CMAKE_CXX_ABI_COMPILED 1)

# Set Wine prefix
if(NOT DEFINED ENV{WINE_PREFIX})
    set(WINE_PREFIX "$ENV{HOME}/.wine")
else()
    set(WINE_PREFIX "$ENV{WINE_PREFIX}")
endif()

# Compiler-specific setup
if(WINE_COMPILER STREQUAL "MSVC")
    # MSVC setup
    if(DEFINED ENV{MSVC_WINE_CL})
        set(CMAKE_C_COMPILER "$ENV{MSVC_WINE_CL}")
        set(CMAKE_CXX_COMPILER "$ENV{MSVC_WINE_CL}")
    else()
        find_program(MSVC_WINE_CL cl)
        if(NOT MSVC_WINE_CL)
            message(FATAL_ERROR "cl not found in PATH. Please ensure msvc-wine is installed and in PATH")
        endif()
        set(CMAKE_C_COMPILER "${MSVC_WINE_CL}")
        set(CMAKE_CXX_COMPILER "${MSVC_WINE_CL}")
    endif()
    
    # MSVC tools
    set(CMAKE_RC_COMPILER rc)
    set(CMAKE_AR lib)
    set(CMAKE_LINKER link)

    find_program(MSVC_ML64 ml64)
    if(MSVC_ML64)
        set(CMAKE_ASM_MASM_COMPILER ${MSVC_ML64})
    else()
        # If ml64 not in PATH, look in the standard msvc-wine location
        set(CMAKE_ASM_MASM_COMPILER "${HOME}/my_msvc/opt/msvc/bin/x64/ml64")
    endif()
    
    # Identify as MSVC
    set(MSVC 1)
    set(CMAKE_C_COMPILER_ID "MSVC" CACHE STRING "" FORCE)
    set(CMAKE_CXX_COMPILER_ID "MSVC" CACHE STRING "" FORCE)
    
    # Force MSVC version to avoid detection issues with Wine
    # This corresponds to VS2022 17.12 (latest as of writing)
    set(CMAKE_C_COMPILER_VERSION "19.44.35213.0" CACHE STRING "" FORCE)
    set(CMAKE_CXX_COMPILER_VERSION "19.44.35213.0" CACHE STRING "" FORCE)
    set(MSVC_VERSION 1944 CACHE STRING "" FORCE)
    
    # Set C++ standard defaults for MSVC
    # These are internal CMake variables that must be set to avoid detection issues
    set(CMAKE_CXX_STANDARD_COMPUTED_DEFAULT "14" CACHE INTERNAL "")
    set(CMAKE_CXX_EXTENSIONS_COMPUTED_DEFAULT "OFF" CACHE INTERNAL "")
    set(CMAKE_C_STANDARD_COMPUTED_DEFAULT "11" CACHE INTERNAL "")
    set(CMAKE_C_EXTENSIONS_COMPUTED_DEFAULT "OFF" CACHE INTERNAL "")
    
    # MSVC flags
    set(CMAKE_C_FLAGS_INIT "/DWIN32 /D_WINDOWS")
    set(CMAKE_CXX_FLAGS_INIT "/DWIN32 /D_WINDOWS /D_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR /EHsc")
    
elseif(WINE_COMPILER STREQUAL "CLANG_CL")
    # clang-cl setup
    if(DEFINED ENV{CLANG_CL_WINE})
        set(CMAKE_C_COMPILER "$ENV{CLANG_CL_WINE}")
        set(CMAKE_CXX_COMPILER "$ENV{CLANG_CL_WINE}")
    else()
        find_program(CLANG_CL_WINE clang-cl)
        if(NOT CLANG_CL_WINE)
            message(FATAL_ERROR "clang-cl not found. Please install Windows LLVM in Wine or provide CLANG_CL_WINE")
        endif()
        set(CMAKE_C_COMPILER "${CLANG_CL_WINE}")
        set(CMAKE_CXX_COMPILER "${CLANG_CL_WINE}")
    endif()
    
    # clang-cl tools
    set(CMAKE_RC_COMPILER rc)
    set(CMAKE_AR llvm-lib)
    set(CMAKE_LINKER lld-link)

    # Tell CMake we're using lld-link, not MSVC link.exe
    # This prevents it from using MSVC-specific linker flags
    set(CMAKE_LINKER_TYPE LLD)
    
    # Identify as Clang with MSVC compatibility
    set(CMAKE_C_COMPILER_ID "Clang")
    set(CMAKE_CXX_COMPILER_ID "Clang")
    set(CMAKE_C_COMPILER_FRONTEND_VARIANT "MSVC")
    set(CMAKE_CXX_COMPILER_FRONTEND_VARIANT "MSVC")

    # Find msvc-wine installation
    # Standard msvc-wine locations as per their README
    if(DEFINED ENV{MSVC_WINE_PREFIX})
        set(MSVC_BASE "$ENV{MSVC_WINE_PREFIX}")
    elseif(EXISTS "$ENV{HOME}/my_msvc/opt/msvc")
        # Standard location from msvc-wine README
        set(MSVC_BASE "$ENV{HOME}/my_msvc/opt/msvc")
    elseif(EXISTS "$ENV{HOME}/.msvc-wine/opt/msvc")
        set(MSVC_BASE "$ENV{HOME}/.msvc-wine/opt/msvc")
    else()
        message(FATAL_ERROR "msvc-wine not found. Please install following msvc-wine README or set MSVC_WINE_PREFIX")
    endif()
    
    message(STATUS "Using msvc-wine at ${MSVC_BASE}")

    set(CMAKE_ASM_MASM_COMPILER "${MSVC_BASE}/bin/x64/ml64")
    
    # Find the MSVC version directory
    file(GLOB MSVC_VERSION_DIRS "${MSVC_BASE}/VC/Tools/MSVC/*")
    if(NOT MSVC_VERSION_DIRS)
        message(FATAL_ERROR "No MSVC version found in ${MSVC_BASE}/VC/Tools/MSVC/")
    endif()
    list(GET MSVC_VERSION_DIRS 0 MSVC_TOOLS_DIR)
    get_filename_component(MSVC_VERSION "${MSVC_TOOLS_DIR}" NAME)
    message(STATUS "Found MSVC version: ${MSVC_VERSION}")
    
    # Find Windows SDK version
    file(GLOB SDK_VERSION_DIRS "${MSVC_BASE}/Windows Kits/10/Include/*")
    if(SDK_VERSION_DIRS)
        list(GET SDK_VERSION_DIRS 0 SDK_VERSION_PATH)
        get_filename_component(SDK_VERSION "${SDK_VERSION_PATH}" NAME)
        set(SDK_BASE "${MSVC_BASE}/Windows Kits/10")
        message(STATUS "Found Windows SDK version: ${SDK_VERSION}")
    else()
        message(WARNING "Windows SDK not found")
    endif()
    
    # Get clang resource directory for intrinsics
    execute_process(
        COMMAND ${CMAKE_C_COMPILER} /clang:-print-resource-dir
        OUTPUT_VARIABLE CLANG_RESOURCE_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    
    # Build include paths - /imsvc flag concatenates directly with path (no space)
    # For paths with spaces, we need to quote the entire argument
    set(CMAKE_C_FLAGS_INIT "/DWIN32 /D_WINDOWS")
    set(CMAKE_CXX_FLAGS_INIT "/DWIN32 /D_WINDOWS /D_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR /EHsc /std:c++23")
    
    # Add include paths - quote entire /imsvc argument if path contains spaces
    # Clang intrinsics (must be first for SSE/AVX support)
    if(CLANG_RESOURCE_DIR)
        if(CLANG_RESOURCE_DIR MATCHES " ")
            set(CMAKE_C_FLAGS_INIT "${CMAKE_C_FLAGS_INIT} \"/imsvc${CLANG_RESOURCE_DIR}/include\"")
            set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} \"/imsvc${CLANG_RESOURCE_DIR}/include\"")
        else()
            set(CMAKE_C_FLAGS_INIT "${CMAKE_C_FLAGS_INIT} /imsvc${CLANG_RESOURCE_DIR}/include")
            set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} /imsvc${CLANG_RESOURCE_DIR}/include")
        endif()
    endif()
    
    # MSVC includes
    if(MSVC_TOOLS_DIR MATCHES " ")
        set(CMAKE_C_FLAGS_INIT "${CMAKE_C_FLAGS_INIT} \"/imsvc${MSVC_TOOLS_DIR}/include\"")
        set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} \"/imsvc${MSVC_TOOLS_DIR}/include\"")
    else()
        set(CMAKE_C_FLAGS_INIT "${CMAKE_C_FLAGS_INIT} /imsvc${MSVC_TOOLS_DIR}/include")
        set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} /imsvc${MSVC_TOOLS_DIR}/include")
    endif()
    
    # Windows SDK includes - these WILL have spaces because of "Windows Kits"
    if(DEFINED SDK_BASE)
        # All SDK paths need quoting due to "Windows Kits" space
        set(CMAKE_C_FLAGS_INIT "${CMAKE_C_FLAGS_INIT} \"/imsvc${SDK_BASE}/Include/${SDK_VERSION}/ucrt\"")
        set(CMAKE_C_FLAGS_INIT "${CMAKE_C_FLAGS_INIT} \"/imsvc${SDK_BASE}/Include/${SDK_VERSION}/shared\"")
        set(CMAKE_C_FLAGS_INIT "${CMAKE_C_FLAGS_INIT} \"/imsvc${SDK_BASE}/Include/${SDK_VERSION}/um\"")
        set(CMAKE_C_FLAGS_INIT "${CMAKE_C_FLAGS_INIT} \"/imsvc${SDK_BASE}/Include/${SDK_VERSION}/winrt\"")
        
        set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} \"/imsvc${SDK_BASE}/Include/${SDK_VERSION}/ucrt\"")
        set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} \"/imsvc${SDK_BASE}/Include/${SDK_VERSION}/shared\"")
        set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} \"/imsvc${SDK_BASE}/Include/${SDK_VERSION}/um\"")
        set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} \"/imsvc${SDK_BASE}/Include/${SDK_VERSION}/winrt\"")
    endif()
    
    # Set up library paths - quote entire /LIBPATH: argument if path contains spaces
    set(LINKER_FLAGS "")
    
    # MSVC libraries
    if(MSVC_TOOLS_DIR MATCHES " ")
        set(LINKER_FLAGS "${LINKER_FLAGS} \"/LIBPATH:${MSVC_TOOLS_DIR}/lib/x64\"")
    else()
        set(LINKER_FLAGS "${LINKER_FLAGS} /LIBPATH:${MSVC_TOOLS_DIR}/lib/x64")
    endif()
    
    # Windows SDK libraries - need quoting due to "Windows Kits" space
    if(DEFINED SDK_BASE)
        set(LINKER_FLAGS "${LINKER_FLAGS} \"/LIBPATH:${SDK_BASE}/Lib/${SDK_VERSION}/ucrt/x64\"")
        set(LINKER_FLAGS "${LINKER_FLAGS} \"/LIBPATH:${SDK_BASE}/Lib/${SDK_VERSION}/um/x64\"")
    endif()
    
    # Apply linker flags to CMake linker variables
    set(CMAKE_EXE_LINKER_FLAGS_INIT "${LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS_INIT "${LINKER_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS_INIT "${LINKER_FLAGS}")
    set(CMAKE_STATIC_LINKER_FLAGS_INIT "")

    string(REPLACE "/INCREMENTAL:YES" "" CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
    string(REPLACE "/INCREMENTAL:YES" "" CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG}")
    
    # Standard Windows libraries
    set(CMAKE_C_STANDARD_LIBRARIES "kernel32.lib user32.lib gdi32.lib winspool.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib oldnames.lib" CACHE STRING "" FORCE)
    set(CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES}" CACHE STRING "" FORCE)
    
    # C++ standard settings
    set(CMAKE_CXX_STANDARD 23 CACHE STRING "C++ standard" FORCE)
    set(CMAKE_CXX_STANDARD_REQUIRED ON CACHE BOOL "C++ standard required" FORCE)
    
    # Manually set C++ standard support to bypass feature detection
    set(CMAKE_CXX_COMPILE_FEATURES 
        cxx_std_98 cxx_std_11 cxx_std_14 cxx_std_17 cxx_std_20 cxx_std_23
        CACHE INTERNAL "C++ compile features"
    )
endif()

# Common Wine settings
# Disable Wine debug output for cleaner builds
set(ENV{WINEDEBUG} "-all")

# Common library prefixes and suffixes
set(CMAKE_IMPORT_LIBRARY_PREFIX "")
set(CMAKE_IMPORT_LIBRARY_SUFFIX ".lib")
set(CMAKE_STATIC_LIBRARY_PREFIX "")
set(CMAKE_STATIC_LIBRARY_SUFFIX ".lib")
set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_SHARED_LIBRARY_SUFFIX ".dll")
set(CMAKE_EXECUTABLE_SUFFIX ".exe")

# Common find root path settings
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Export compile commands for better IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)