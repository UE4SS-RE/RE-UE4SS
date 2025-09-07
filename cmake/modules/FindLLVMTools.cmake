# FindLLVMTools.cmake - Helper module for finding LLVM/Clang tools
#
# This module provides functions to find LLVM tools with version suffixes
# and allows user overrides via cache variables.
#
# Provides:
#   find_llvm_tool() - Function to find LLVM tools with version fallbacks
#
# Sets:
#   Various cache variables for tool paths (can be overridden by user)

# Common LLVM version suffixes to search
set(LLVM_VERSION_SUFFIXES 22 20 19 18 17 16 15)

# Additional search paths for special environments
set(LLVM_SEARCH_PATHS "")
if(DEFINED ENV{LLVM_ROOT})
    list(APPEND LLVM_SEARCH_PATHS "$ENV{LLVM_ROOT}/bin")
endif()
if(WIN32)
    list(APPEND LLVM_SEARCH_PATHS 
        "$ENV{ProgramFiles}/LLVM/bin"
        "$ENV{ProgramFiles\(x86\)}/LLVM/bin"
    )
endif()

# Finds an LLVM tool with version fallbacks
#
# This function searches for an LLVM tool by name, trying version suffixes
# if the unversioned tool is not found. Results are cached for user override.
#
# Arguments:
#   RESULT_VAR - Variable name to store the result
#   TOOL_NAME - Base name of the tool (e.g., "clang-cl", "lld-link")
#   DESCRIPTION - Description for the cache variable
#   REQUIRED - If TRUE, fail if tool not found
#
# Example usage:
#   find_llvm_tool(CLANG_CL_EXECUTABLE "clang-cl" "Clang-CL compiler" TRUE)
#
function(find_llvm_tool RESULT_VAR TOOL_NAME DESCRIPTION REQUIRED)
    # Check if user has already set this variable
    if(DEFINED ${RESULT_VAR} AND ${RESULT_VAR})
        return()
    endif()
    
    # Build list of names to search
    set(SEARCH_NAMES ${TOOL_NAME})
    
    # Add .exe suffix on Windows
    if(WIN32 OR CMAKE_HOST_WIN32)
        list(APPEND SEARCH_NAMES "${TOOL_NAME}.exe")
    endif()
    
    # Add version suffixes
    foreach(VERSION ${LLVM_VERSION_SUFFIXES})
        list(APPEND SEARCH_NAMES "${TOOL_NAME}-${VERSION}")
        if(WIN32 OR CMAKE_HOST_WIN32)
            list(APPEND SEARCH_NAMES "${TOOL_NAME}-${VERSION}.exe")
        endif()
    endforeach()
    
    # Search for the tool
    find_program(${RESULT_VAR}
        NAMES ${SEARCH_NAMES}
        PATHS ${LLVM_SEARCH_PATHS}
        DOC ${DESCRIPTION}
    )
    
    # Handle required flag
    if(REQUIRED AND NOT ${RESULT_VAR})
        message(STATUS "Search paths: ${LLVM_SEARCH_PATHS}")
        message(STATUS "Search names: ${SEARCH_NAMES}")
        message(FATAL_ERROR "Could not find required tool: ${TOOL_NAME}")
    endif()
    
    # Make it a cache variable so user can override
    set(${RESULT_VAR} "${${RESULT_VAR}}" CACHE FILEPATH ${DESCRIPTION})
endfunction()

# Finds common LLVM/Clang tools
#
# This unified function finds LLVM tools based on what's needed.
# Tools are only searched for if the corresponding option is TRUE.
#
# Arguments (all optional, default FALSE):
#   NEED_CLANG - Find clang/clang++
#   NEED_CLANG_CL - Find clang-cl
#   NEED_LLD - Find lld-link
#   NEED_LLVM_LIB - Find llvm-lib/llvm-ar
#   NEED_LLVM_RC - Find llvm-rc
#   NEED_LLVM_MT - Find llvm-mt
#   NEED_LLVM_RANLIB - Find llvm-ranlib
#
# Sets in parent scope (if requested):
#   CLANG_EXECUTABLE - Path to clang
#   CLANGXX_EXECUTABLE - Path to clang++
#   CLANG_CL_EXECUTABLE - Path to clang-cl
#   LLD_LINK_EXECUTABLE - Path to lld-link
#   LLVM_LIB_EXECUTABLE - Path to llvm-lib
#   LLVM_AR_EXECUTABLE - Path to llvm-ar
#   LLVM_RC_EXECUTABLE - Path to llvm-rc
#   LLVM_MT_EXECUTABLE - Path to llvm-mt
#   LLVM_RANLIB_EXECUTABLE - Path to llvm-ranlib
#
# Example usage:
#   find_llvm_tools(NEED_CLANG_CL TRUE NEED_LLD TRUE NEED_LLVM_LIB TRUE)
#
function(find_llvm_tools)
    cmake_parse_arguments(FIND_LLVM
        ""
        ""
        "NEED_CLANG;NEED_CLANG_CL;NEED_LLD;NEED_LLVM_LIB;NEED_LLVM_RC;NEED_LLVM_MT;NEED_LLVM_RANLIB"
        ${ARGN}
    )
    
    # Find requested tools
    if(FIND_LLVM_NEED_CLANG)
        find_llvm_tool(CLANG_EXECUTABLE "clang" "Clang C compiler" TRUE)
        find_llvm_tool(CLANGXX_EXECUTABLE "clang++" "Clang C++ compiler" TRUE)
        set(CLANG_EXECUTABLE "${CLANG_EXECUTABLE}" PARENT_SCOPE)
        set(CLANGXX_EXECUTABLE "${CLANGXX_EXECUTABLE}" PARENT_SCOPE)
    endif()
    
    if(FIND_LLVM_NEED_CLANG_CL)
        find_llvm_tool(CLANG_CL_EXECUTABLE "clang-cl" "Clang-CL compiler" TRUE)
        set(CLANG_CL_EXECUTABLE "${CLANG_CL_EXECUTABLE}" PARENT_SCOPE)
    endif()
    
    if(FIND_LLVM_NEED_LLD)
        find_llvm_tool(LLD_LINK_EXECUTABLE "lld-link" "LLD linker" TRUE)
        set(LLD_LINK_EXECUTABLE "${LLD_LINK_EXECUTABLE}" PARENT_SCOPE)
    endif()
    
    if(FIND_LLVM_NEED_LLVM_LIB)
        # Try llvm-lib first (Windows style), then llvm-ar (Unix style)
        find_llvm_tool(LLVM_LIB_EXECUTABLE "llvm-lib" "LLVM librarian" FALSE)
        if(NOT LLVM_LIB_EXECUTABLE)
            find_llvm_tool(LLVM_AR_EXECUTABLE "llvm-ar" "LLVM archiver" TRUE)
            set(LLVM_LIB_EXECUTABLE "${LLVM_AR_EXECUTABLE}")
        endif()
        set(LLVM_LIB_EXECUTABLE "${LLVM_LIB_EXECUTABLE}" PARENT_SCOPE)
        set(LLVM_AR_EXECUTABLE "${LLVM_AR_EXECUTABLE}" PARENT_SCOPE)
    endif()
    
    if(FIND_LLVM_NEED_LLVM_RC)
        find_llvm_tool(LLVM_RC_EXECUTABLE "llvm-rc" "LLVM resource compiler" TRUE)
        set(LLVM_RC_EXECUTABLE "${LLVM_RC_EXECUTABLE}" PARENT_SCOPE)
    endif()
    
    if(FIND_LLVM_NEED_LLVM_MT)
        find_llvm_tool(LLVM_MT_EXECUTABLE "llvm-mt" "LLVM manifest tool" FALSE)
        set(LLVM_MT_EXECUTABLE "${LLVM_MT_EXECUTABLE}" PARENT_SCOPE)
    endif()
    
    if(FIND_LLVM_NEED_LLVM_RANLIB)
        find_llvm_tool(LLVM_RANLIB_EXECUTABLE "llvm-ranlib" "LLVM ranlib" TRUE)
        set(LLVM_RANLIB_EXECUTABLE "${LLVM_RANLIB_EXECUTABLE}" PARENT_SCOPE)
    endif()
endfunction()

# Convenience wrapper for Windows-targeting toolchains
function(find_llvm_windows_tools)
    find_llvm_tools(
        NEED_CLANG_CL TRUE
        NEED_LLD TRUE
        NEED_LLVM_LIB TRUE
        NEED_LLVM_RC TRUE
        NEED_LLVM_MT TRUE
        NEED_LLVM_RANLIB TRUE
    )
    # Forward all variables to parent scope
    set(CLANG_CL_EXECUTABLE "${CLANG_CL_EXECUTABLE}" PARENT_SCOPE)
    set(LLD_LINK_EXECUTABLE "${LLD_LINK_EXECUTABLE}" PARENT_SCOPE)
    set(LLVM_LIB_EXECUTABLE "${LLVM_LIB_EXECUTABLE}" PARENT_SCOPE)
    set(LLVM_RC_EXECUTABLE "${LLVM_RC_EXECUTABLE}" PARENT_SCOPE)
    set(LLVM_MT_EXECUTABLE "${LLVM_MT_EXECUTABLE}" PARENT_SCOPE)
    set(LLVM_RANLIB_EXECUTABLE "${LLVM_RANLIB_EXECUTABLE}" PARENT_SCOPE)
endfunction()

# Convenience wrapper for GNU-style toolchains
function(find_llvm_gnu_tools)
    find_llvm_tools(
        NEED_CLANG TRUE
        NEED_LLD TRUE
        NEED_LLVM_RC FALSE
    )
    # Forward all variables to parent scope
    set(CLANG_EXECUTABLE "${CLANG_EXECUTABLE}" PARENT_SCOPE)
    set(CLANGXX_EXECUTABLE "${CLANGXX_EXECUTABLE}" PARENT_SCOPE)
    set(LLD_LINK_EXECUTABLE "${LLD_LINK_EXECUTABLE}" PARENT_SCOPE)
    set(LLVM_RC_EXECUTABLE "${LLVM_RC_EXECUTABLE}" PARENT_SCOPE)
endfunction()