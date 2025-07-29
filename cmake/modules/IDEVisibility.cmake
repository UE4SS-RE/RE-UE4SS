# CMake module for IDE visibility helpers
# A lightweight, decentralized approach to making source files visible in IDEs

# Global option (already defined in main CMakeLists.txt)
# option(ENABLE_IDE_SOURCE_VISIBILITY "Enable IDE visibility for source files" ON)

# Helper function to make header files visible in the IDE
# To be called from individual component CMakeLists.txt files
#
# Arguments:
#   TARGET_NAME - Name of the target to add headers to
#   INCLUDE_DIR - Directory containing header files
#
# Example usage:
#   make_headers_visible(MyLib "${CMAKE_CURRENT_SOURCE_DIR}/include")
#
function(make_headers_visible TARGET_NAME INCLUDE_DIR)
    if(NOT DEFINED ENABLE_IDE_SOURCE_VISIBILITY OR NOT ENABLE_IDE_SOURCE_VISIBILITY)
        return()
    endif()
    
    if(NOT TARGET ${TARGET_NAME} OR NOT EXISTS "${INCLUDE_DIR}")
        return()
    endif()
    
    # Find all header files
    file(GLOB_RECURSE HEADER_FILES
        "${INCLUDE_DIR}/*.h"
        "${INCLUDE_DIR}/*.hpp"
        "${INCLUDE_DIR}/*.hxx"
        "${INCLUDE_DIR}/*.inl"
    )
    
    if(HEADER_FILES)
        # Check if it's an INTERFACE library or not
        get_target_property(target_type ${TARGET_NAME} TYPE)
        
        if(target_type STREQUAL "INTERFACE_LIBRARY")
            # For INTERFACE libraries
            target_sources(${TARGET_NAME} INTERFACE
                FILE_SET headers TYPE HEADERS
                BASE_DIRS ${INCLUDE_DIR}
                FILES ${HEADER_FILES}
            )
        else()
            # For normal libraries
            target_sources(${TARGET_NAME} PUBLIC
                FILE_SET headers TYPE HEADERS
                BASE_DIRS ${INCLUDE_DIR}
                FILES ${HEADER_FILES}
            )
        endif()
        
        message(STATUS "Added ${TARGET_NAME} headers to IDE visibility")
    endif()
endfunction()

# Helper function to make Rust files visible in the IDE
# Creates lightweight OBJECT libraries that don't get built
#
# Arguments:
#   TARGET_NAME - Name for the visibility target (must not already exist)
#   SOURCE_DIR - Directory containing Rust source files
#   FOLDER_PATH - IDE folder path for organization
#
# Example usage:
#   make_rust_visible(MyRustLib_IDE "${CMAKE_CURRENT_SOURCE_DIR}/src" "deps/first/MyRustLib")
#
function(make_rust_visible TARGET_NAME SOURCE_DIR FOLDER_PATH)
    if(NOT DEFINED ENABLE_IDE_SOURCE_VISIBILITY OR NOT ENABLE_IDE_SOURCE_VISIBILITY)
        return()
    endif()
    
    if(TARGET ${TARGET_NAME} OR NOT EXISTS "${SOURCE_DIR}")
        return()
    endif()
    
    # Find Rust source files
    file(GLOB_RECURSE RUST_FILES "${SOURCE_DIR}/*.rs")
    
    if(RUST_FILES)
        # Create a minimal OBJECT library
        add_library(${TARGET_NAME} OBJECT EXCLUDE_FROM_ALL)
        
        # Add Rust files as sources
        target_sources(${TARGET_NAME} PRIVATE ${RUST_FILES})
        
        # Organize files using source_group
        source_group(TREE "${SOURCE_DIR}" PREFIX "Source Files" FILES ${RUST_FILES})
        
        # Set folder properties for IDE organization
        set_target_properties(${TARGET_NAME} PROPERTIES 
            FOLDER "${FOLDER_PATH}"
            VS_FOLDER "${FOLDER_PATH}"
            LINKER_LANGUAGE CXX)
            
        message(STATUS "Created ${TARGET_NAME} for Rust files IDE visibility")
    endif()
endfunction()
