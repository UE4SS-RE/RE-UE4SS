include(FetchContent)

project(LuaJIT)

set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(LuaJIT
    GIT_REPOSITORY git@github.com:LuaJIT/LuaJIT.git
    GIT_TAG v2.1.ROLLING
    GIT_PROGRESS ON
    CONFIGURE_COMMAND ""
    PATCH_COMMAND cmake -E copy_if_different "${CMAKE_BINARY_DIR}/_deps/luajit-src/src/luajit_rolling.h" "${CMAKE_BINARY_DIR}/_deps/luajit-src/src/luajit.h"
    COMMAND
    PATCH_COMMAND echo HELLO)

FetchContent_Populate(LuaJIT)
FetchContent_GetProperties(LuaJIT SOURCE_DIR LuaJIT_FETCHED_SOURCE)

# Hack for not being able to compile luajit -> START
FetchContent_Declare(LuaJIT_bin_jit
    URL https://github.com/UE4SS/LuaJIT/releases/download/bins/luajit.lib
    DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/_deps/luajit-src/src"
    DOWNLOAD_NO_EXTRACT TRUE)
FetchContent_Declare(LuaJIT_bin_lua51
    URL https://github.com/UE4SS/LuaJIT/releases/download/bins/lua51.lib
    DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/_deps/luajit-src/src"
    DOWNLOAD_NO_EXTRACT TRUE)
FetchContent_MakeAvailable(LuaJIT_bin_jit LuaJIT_bin_lua51)
# Hack for not being able to compile luajit -> END
