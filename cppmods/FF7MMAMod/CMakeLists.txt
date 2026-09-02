project(FF7MMAMod)

set(TARGET FF7MMAMod)

set(${TARGET}_Sources
    "${CMAKE_CURRENT_SOURCE_DIR}/src/dllmain.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/DataTable.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Writer.cpp"
)

set(${TARGET}_Headers
    "${CMAKE_CURRENT_SOURCE_DIR}/include/FF7MMAMod.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/MMA/Types.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/MMA/FrozenReader.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/MMA/DataTable.hpp"
)

add_library(${TARGET} SHARED ${${TARGET}_Sources} ${${TARGET}_Headers})

target_include_directories(${TARGET} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# Link against UE4SS (provides Unreal types, ImGui, etc.)
target_link_libraries(${TARGET} PUBLIC UE4SS)

# Link against ImGui for UI
target_link_libraries(${TARGET} PRIVATE ImGui)

# JSON handling for the data mod pipeline
target_link_libraries(${TARGET} PRIVATE glaze::glaze)

# Set folder for IDE organization
set_target_properties(${TARGET} PROPERTIES FOLDER "mods")

# Deploy to game directory after build
set(FF7R_MOD_DIR "F:/SteamLibrary/steamapps/common/FINAL FANTASY VII REBIRTH/End/Binaries/Win64/ue4ss/Mods/FF7MMAMod/dlls")

add_custom_command(TARGET ${TARGET} POST_BUILD
    # Create the mod directory if it doesn't exist
    COMMAND ${CMAKE_COMMAND} -E make_directory "${FF7R_MOD_DIR}"
    # Remove old main.dll if it exists
    COMMAND ${CMAKE_COMMAND} -E remove -f "${FF7R_MOD_DIR}/main.dll"
    # Copy the built DLL as main.dll
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${TARGET}>" "${FF7R_MOD_DIR}/main.dll"
    COMMENT "Deploying ${TARGET} to FF7 Rebirth mod directory..."
)
