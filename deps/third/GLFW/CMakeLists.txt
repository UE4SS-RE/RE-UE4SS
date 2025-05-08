include(FetchContent)

set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.9
    GIT_SHALLOW TRUE
    GIT_PROGRESS ON
)

# Disable unnecessary components
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(glfw)

# Set GLFW targets to be in deps/third/GLFW folder
# Uses set_folder_for_target() from cmake/modules/IDEOrganization.cmake
if(TARGET glfw)
    set_folder_for_target(glfw "deps/third/GLFW")
endif()

if(TARGET update_mappings)
    set_folder_for_target(update_mappings "deps/third/GLFW")
endif()

if(TARGET uninstall)
    set_folder_for_target(uninstall "deps/third/GLFW")
endif()
