include(FetchContent)

set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.9
    GIT_SHALLOW TRUE
    GIT_PROGRESS ON
)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(glfw)

# Set GLFW targets to be in deps/third/GLFW folder
if(TARGET glfw)
    set_target_properties(glfw PROPERTIES FOLDER "deps/third/GLFW")
endif()

if(TARGET update_mappings)
    set_target_properties(update_mappings PROPERTIES FOLDER "deps/third/GLFW")
endif()

if(TARGET uninstall)
    set_target_properties(uninstall PROPERTIES FOLDER "deps/third/GLFW")
endif()
