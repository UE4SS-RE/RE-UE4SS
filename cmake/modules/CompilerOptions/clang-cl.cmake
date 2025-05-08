# Compiler options for clang-cl (Clang with MSVC-compatible frontend)
# This file can be used to add clang-cl specific options that differ from MSVC

# Start with MSVC base flags
include("${CMAKE_CURRENT_LIST_DIR}/msvc.cmake")

# Remove flags that clang-cl doesn't support
string(REPLACE "/MP;" "" DEFAULT_COMPILER_FLAGS "${DEFAULT_COMPILER_FLAGS}")
string(REPLACE "/MP" "" DEFAULT_COMPILER_FLAGS "${DEFAULT_COMPILER_FLAGS}")
string(REPLACE "/Zc:preprocessor;" "" DEFAULT_COMPILER_FLAGS "${DEFAULT_COMPILER_FLAGS}")
string(REPLACE "/Zc:preprocessor" "" DEFAULT_COMPILER_FLAGS "${DEFAULT_COMPILER_FLAGS}")

# Set the cleaned flags in parent scope
set(DEFAULT_COMPILER_FLAGS "${DEFAULT_COMPILER_FLAGS}" PARENT_SCOPE)

# clang-cl specific optimizations could go here
# set(Shipping_FLAGS "${Shipping_FLAGS};-flto=thin" PARENT_SCOPE)