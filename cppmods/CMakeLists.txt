# Enable IDE organization with folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Add C++ mods
add_subdirectory("KismetDebuggerMod")
add_subdirectory("EventViewerMod")

# Opt-in harness for validating a generated SDK against live reflection. Keeping this behind a
# path avoids coupling ordinary UE4SS builds to a machine-local generated tree.
if(UE4SS_SDK_RUNTIME_TEST_ROOT)
    add_subdirectory("SDKRuntimeTestMod")
endif()

# Organize targets in the "mods" folder
get_property(TARGETS DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS)
foreach(target ${TARGETS})
    set_target_properties(${target} PROPERTIES FOLDER "mods")
endforeach()
