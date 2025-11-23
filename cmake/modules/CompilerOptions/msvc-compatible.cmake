# MSVC-compatible compiler options (for both MSVC and clang-cl)
#
# This file configures compiler options for MSVC and clang-cl compilers,
# which share the same command-line interface and flags.

# Base flags common to both MSVC and clang-cl
set(DEFAULT_COMPILER_FLAGS "/W3;/wd4005;/wd4251;/wd4068;/Zc:inline;/Zc:strictStrings;/Gy;/D_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR")

# Determine if we're using clang-cl
get_filename_component(COMPILER_NAME "${CMAKE_CXX_COMPILER}" NAME)
if(COMPILER_NAME MATCHES "clang-cl" OR 
   (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
    set(IS_CLANG_CL TRUE)
else()
    set(IS_CLANG_CL FALSE)
endif()

# Add MSVC-specific flags that clang-cl doesn't support
if(NOT IS_CLANG_CL)
    list(APPEND DEFAULT_COMPILER_FLAGS "/MP" "/Zc:preprocessor")
endif()

set(DEFAULT_COMPILER_FLAGS ${DEFAULT_COMPILER_FLAGS} PARENT_SCOPE)

# Default linker flags - always include debug info for all configs
set(DEFAULT_SHARED_LINKER_FLAGS "" PARENT_SCOPE)
set(DEFAULT_EXE_LINKER_FLAGS "/DEBUG:FULL" PARENT_SCOPE)

# Configuration-specific compiler flags
set(Debug_FLAGS "" PARENT_SCOPE)  # Will use defaults from CMAKE_CXX_FLAGS_DEBUG
set(Dev_FLAGS "" PARENT_SCOPE)    # Will use defaults from CMAKE_CXX_FLAGS_DEBUG
set(Test_FLAGS "" PARENT_SCOPE)   # Will use defaults from CMAKE_CXX_FLAGS_RELWITHDEBINFO
set(Shipping_FLAGS "/Zi" PARENT_SCOPE)  # Add debug symbols to release

# Configuration-specific linker flags
# Debug/Dev: Keep incremental linking for faster iteration
set(Debug_LINKER_FLAGS "" PARENT_SCOPE)
set(Dev_LINKER_FLAGS "" PARENT_SCOPE)

# Test: Keep incremental but include additional optimizations
set(Test_LINKER_FLAGS "/OPT:REF;/OPT:ICF" PARENT_SCOPE)

# Shipping: Disable incremental linking, enable optimizations
# Note: /OPT:REF and /OPT:ICF are disabled by default when /DEBUG is used,
# so we must explicitly enable them for release builds
set(Shipping_LINKER_FLAGS "/LTCG;/INCREMENTAL:NO;/OPT:REF;/OPT:ICF" PARENT_SCOPE)