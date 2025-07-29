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

# Make the content available
FetchContent_MakeAvailable(glfw)

# Suppress warnings from GLFW
# Uses suppress_third_party_warnings() from cmake/modules/ThirdPartyWarnings.cmake
suppress_third_party_warnings(glfw)

# Set the folder for GLFW targets right after they're created
# This happens before any other organization attempts
if(TARGET glfw)
    set_target_properties(glfw PROPERTIES FOLDER "deps/third/GLFW3")
endif()

if(TARGET glfw3)
    set_target_properties(glfw3 PROPERTIES FOLDER "deps/third/GLFW3")
endif()

if(TARGET update_mappings)
    set_target_properties(update_mappings PROPERTIES FOLDER "deps/third/GLFW3")
endif()

if(TARGET uninstall)
    set_target_properties(uninstall PROPERTIES FOLDER "deps/third/GLFW3")
endif()
