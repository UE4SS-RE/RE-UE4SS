cmake_minimum_required(VERSION 3.18)

set(TARGET LuaRaw)
project(${TARGET})

set(${TARGET}_Sources
        "${CMAKE_CURRENT_SOURCE_DIR}/src/lapi.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/lauxlib.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/lbaselib.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/lcode.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/lcorolib.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/lctype.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/ldblib.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/ldebug.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/ldo.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/ldump.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/lfunc.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/lgc.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/linit.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/liolib.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/llex.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/lmathlib.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/lmem.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/loadlib.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/lobject.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/lopcodes.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/loslib.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/lparser.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/lstate.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/lstring.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/lstrlib.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/ltable.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/ltablib.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/ltm.c"
        #${CMAKE_CURRENT_SOURCE_DIR}/"src/lua.c"
        #${CMAKE_CURRENT_SOURCE_DIR}/"src/luac.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/luauser.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/lundump.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/lutf8lib.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/lvm.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/lzio.c"
        )

# This makes the .c files be compiled with the C++ compiler
# This causes __cplusplus to be defined which is what Lua uses to determine whether it's compiled as C or C++
set_source_files_properties(${${TARGET}_Sources} PROPERTIES LANGUAGE CXX)

string(REGEX REPLACE "(.)([A-Z])" "\\1_\\2" MODULE_NAME ${TARGET})
string(TOUPPER ${MODULE_NAME} MODULE_NAME)

if ((NOT DEFINED BUILD_SHARED_LIBS) OR (NOT (${BUILD_SHARED_LIBS})))
    set(BUILD_STATIC 1)
    set(BUILD_SHARED 0)
    message("Project: ${TARGET} (STATIC)")
else ()
    set(BUILD_STATIC 0)
    set(BUILD_SHARED 1)
    message("Project: ${TARGET} (SHARED)")
endif ()

add_library(${TARGET} ${${TARGET}_Sources})
target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
# I removed LUA_LIB from the compile definitions because I don't think it's needed but we'll see.
target_compile_definitions(${TARGET} PRIVATE ${PRIVATE_COMPILE_DEFINITIONS} $<${BUILD_SHARED}:LUA_BUILD_AS_DLL>)
target_compile_options(${TARGET} PRIVATE ${PRIVATE_COMPILE_OPTIONS} /W0)
target_compile_features(${TARGET} PUBLIC ${PUBLIC_COMPILE_FEATURES})
target_link_options(${TARGET} PRIVATE ${PRIVATE_LINK_OPTIONS})
