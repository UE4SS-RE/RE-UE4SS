include(FetchContent)

set(FETCHCONTENT_QUIET OFF)

# Third Party Dependencies -> START
if (WIN32 OR UE4SS_USE_GLFW_ON_LINUX)
add_subdirectory("GLFW")
endif()

# glaze JSON library start
FetchContent_Declare(
  glaze
  GIT_REPOSITORY https://github.com/stephenberry/glaze.git
  GIT_TAG v1.5.5
  GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(glaze)
# glaze JSON library end

if (WIN32 OR UE4SS_USE_GLFW_ON_LINUX)
# ImGui start
FetchContent_Declare(ImGui
    GIT_REPOSITORY git@github.com:ocornut/imgui.git
    GIT_TAG v1.89
    GIT_SHALLOW TRUE
    GIT_PROGRESS ON
    CONFIGURE_COMMAND "")
    
FetchContent_Declare(ImGuiTextEdit
    GIT_REPOSITORY git@github.com:UE4SS-RE/ImGuiColorTextEdit.git
    GIT_TAG master
    GIT_SHALLOW TRUE
    GIT_PROGRESS ON
    CONFIGURE_COMMAND "")
    
FetchContent_Declare(IconFontCppHeaders
    GIT_REPOSITORY git@github.com:juliettef/IconFontCppHeaders.git
    GIT_TAG main
    GIT_SHALLOW TRUE
    GIT_PROGRESS ON
    CONFIGURE_COMMAND "")

add_subdirectory("imgui")
# ImGui end
endif()

# Zydis start
FetchContent_Declare(zydis
    GIT_REPOSITORY git@github.com:zyantific/zydis.git
    GIT_TAG v4.1.0
    GIT_SHALLOW TRUE
    GIT_PROGRESS ON)
    
add_subdirectory("zydis")
# Zydis end

# PolyHook start
FetchContent_Declare(PolyHook2
    GIT_REPOSITORY git@github.com:stevemk14ebr/PolyHook_2_0.git
    GIT_TAG fd2a88f09c8ae89440858fc52573656141013c7f
    GIT_PROGRESS ON)

add_subdirectory("PolyHook_2_0")
# PolyHook end

if (WIN32)
# raw_pdb start
FetchContent_Declare(raw_pdb
    GIT_REPOSITORY git@github.com:MolecularMatters/raw_pdb.git
    GIT_TAG 8c6a7146393c83d27fa101e8bc8017f2a7f151df
    GIT_PROGRESS ON)
    
add_subdirectory("raw_pdb")
# raw_pdb end
endif()

if (LINUX)
# imtui start
FetchContent_Declare(imtui
    GIT_REPOSITORY https://github.com/ggerganov/imtui.git
    GIT_TAG 9f39c3e090c9b1e15557eac38a2f4389462f59df
    PATCH_COMMAND git apply ${CMAKE_CURRENT_SOURCE_DIR}/imtui/fix-size-cjk-and-mouse.patch
    UPDATE_DISCONNECTED 1
    GIT_PROGRESS ON)
FetchContent_Declare(ImGuiTextEdit
    GIT_REPOSITORY git@github.com:UE4SS-RE/ImGuiColorTextEdit.git
    GIT_TAG master
    GIT_SHALLOW TRUE
    GIT_PROGRESS ON
    CONFIGURE_COMMAND "")
add_subdirectory("imtui")
endif()

# fmtlib start
#FetchContent_Declare(fmt
#    GIT_REPOSITORY https://github.com/fmtlib/fmt
#    GIT_TAG 10.2.1
#    GIT_PROGRESS ON)

#add_subdirectory("fmt")
# fmtlib end

# corrosion start
FetchContent_Declare(
    Corrosion
    GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
    GIT_TAG 123be1e3d8170c86e121392e8bffa4def7dc3447)

add_subdirectory("corrosion")
# corrosion end

# Third Party Depenedencies
