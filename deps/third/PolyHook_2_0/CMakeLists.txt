include(FetchContent)

set(FETCHCONTENT_QUIET OFF)

set(ZYDIS_LIBRARY Zydis)
set(ZYCORE_LIBRARY Zycore)

get_target_property(ZYDIS_INCLUDE_DIR ${ZYDIS_LIBRARY} INCLUDE_DIRECTORIES)
get_target_property(ZYCORE_INCLUDE_DIR ${ZYCORE_LIBRARY} INCLUDE_DIRECTORIES)
message(${ZYDIS_INCLUDE_DIR})

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

# add a conditional for disabling 'install' on demand
if(NOT DEFINED _DISABLE_INSTALL)
    set(_DISABLE_INSTALL TRUE)

    macro(install)
        if (NOT _DISABLE_INSTALL)
            _install(${ARGV})
        endif()
    endmacro()
endif()

# prevent PolyHook from setting MSVC_RUNTIME_LIBRARY to "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" by unsetting 'MSVC' var
set(TMP_MSVC ${MSVC})
unset(MSVC)

FetchContent_MakeAvailable(PolyHook2)

# re-enable 'install' and reset 'MSVC' since we're done adding PolyHook
set(MSVC ${TMP_MSVC})
set(_DISABLE_INSTALL FALSE)
