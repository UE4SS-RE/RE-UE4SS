# Enable IDE organization with folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Add C++ mods
add_subdirectory("KismetDebuggerMod")

# Organize targets in the "mods" folder
get_property(TARGETS DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS)
foreach(target ${TARGETS})
    set_target_properties(${target} PROPERTIES FOLDER "mods")
endforeach()
