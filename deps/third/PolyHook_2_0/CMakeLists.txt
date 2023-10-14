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

# add a conditional for disabling 'set_target_properties' on demand to prevent PolyHook from setting MSVC_RUNTIME_LIBRARY to "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
if(NOT DEFINED _DISABLE_SET_PROPERTIES)
    set(_DISABLE_SET_PROPERTIES TRUE)

    macro(set_target_properties)
        if (NOT _DISABLE_SET_PROPERTIES)
            _set_target_properties(${ARGV})
        endif()
    endmacro()
endif()

# add a conditional for disabling 'install' on demand
if(NOT DEFINED _DISABLE_INSTALL)
    set(_DISABLE_INSTALL TRUE)

    macro(install)
        if (NOT _DISABLE_INSTALL)
            _install(${ARGV})
        endif()
    endmacro()
endif()

FetchContent_Declare(PolyHook2
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/PolyHook2
    GIT_REPOSITORY https://github.com/stevemk14ebr/PolyHook_2_0.git
    GIT_TAG 16bc0faf77a15b91b72763353901cfe34daf3174
    GIT_PROGRESS ON)

FetchContent_MakeAvailable(PolyHook2)

# re-enable 'set_target_properties' and 'install' since we're done adding PolyHook
set(_DISABLE_SET_PROPERTIES FALSE)
set(_DISABLE_INSTALL FALSE)
