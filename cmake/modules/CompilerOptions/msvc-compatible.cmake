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

# Linker flags
set(LINKER_FLAGS "/DEBUG:FULL")
set(DEFAULT_SHARED_LINKER_FLAGS "${LINKER_FLAGS}" PARENT_SCOPE)
set(DEFAULT_EXE_LINKER_FLAGS "${LINKER_FLAGS}" PARENT_SCOPE)

# Shipping configuration flags
set(Shipping_FLAGS "/Zi" PARENT_SCOPE)