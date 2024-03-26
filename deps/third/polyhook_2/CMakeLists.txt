cmake_minimum_required(VERSION 3.27)

include(FetchContent)

set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(PolyHook2
    GIT_REPOSITORY https://github.com/stevemk14ebr/PolyHook_2_0.git
    GIT_TAG fd2a88f09c8ae89440858fc52573656141013c7f
    GIT_PROGRESS ON)

if (${UE4SS_PolyHook_BUILD_SHARED})
    set(MAKE_POLYHOOK_DEPENDENCIES_SHARED ON)
    set(MAKE_POLYHOOK_DEPENDENCIES_STATIC OFF)
else ()
    set(MAKE_POLYHOOK_DEPENDENCIES_SHARED OFF)
    set(MAKE_POLYHOOK_DEPENDENCIES_STATIC ON)
endif ()

set(POLYHOOK_BUILD_DLL ON CACHE BOOL "Build dll & lib instead of tests" FORCE)
set(POLYHOOK_BUILD_SHARED_LIB ${MAKE_POLYHOOK_DEPENDENCIES_SHARED} CACHE BOOL "Build polyhook2 as shared libary" FORCE)
set(POLYHOOK_USE_EXTERNAL_ZYDIS ON CACHE BOOL "Use external zydis libary" FORCE)
set(ASMJIT_STATIC ${MAKE_POLYHOOK_DEPENDENCIES_STATIC} CACHE BOOL "Build 'asmjit' library as static" FORCE)
set(POLYHOOK_BUILD_STATIC_RUNTIME OFF CACHE BOOL "Use static runtime" FORCE)

# prevent PolyHook from setting MSVC_RUNTIME_LIBRARY to "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" by unsetting 'MSVC' var
set(TMP_MSVC ${MSVC})
unset(MSVC)

FetchContent_MakeAvailable(PolyHook2)

# re-enable 'install' and reset 'MSVC' since we're done adding PolyHook
set(MSVC ${TMP_MSVC})
