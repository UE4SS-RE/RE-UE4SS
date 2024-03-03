include(FetchContent)

set(FETCHCONTENT_QUIET OFF)

set(ZYDIS_LIBRARY Zydis)
set(ZYCORE_LIBRARY Zycore)

get_target_property(ZYDIS_INCLUDE_DIR ${ZYDIS_LIBRARY} INCLUDE_DIRECTORIES)
get_target_property(ZYCORE_INCLUDE_DIR ${ZYCORE_LIBRARY} INCLUDE_DIRECTORIES)
message(${ZYDIS_INCLUDE_DIR})
# copy setup from polyhook

#if (${UE4SS_PolyHook_BUILD_SHARED})
#    set(DYNOHOOK_BUILD_SHARED_LIB ON)
#else ()
#    set(DYNOHOOK_BUILD_SHARED_LIB OFF)
#endif()

set(DYNOHOOK_USE_EXTERNAL_ZYDIS ON CACHE BOOL "Use external zydis libary" FORCE)
set(DYNOHOOK_BUILD_STATIC_RUNTIME OFF CACHE BOOL "Use static runtime" FORCE)
set(ASMJIT_STATIC ${MAKE_DYNOHOOK_DEPENDENCIES_STATIC} CACHE BOOL "Build 'asmjit' library as static" FORCE)
set(DYNOHOOK_BUILD_STATIC_RUNTIME OFF CACHE BOOL "Use static runtime" FORCE)

if(NOT DEFINED _DISABLE_INSTALL)
    set(_DISABLE_INSTALL TRUE)

    macro(install)
        if (NOT _DISABLE_INSTALL)
            _install(${ARGV})
        endif()
    endmacro()
endif()

FetchContent_MakeAvailable(DynoHook)

set(_DISABLE_INSTALL FALSE)
