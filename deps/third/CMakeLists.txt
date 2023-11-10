include(FetchContent)

set(FETCHCONTENT_QUIET OFF)

# Third Party Dependencies -> START
add_subdirectory("GLFW")

# glaze JSON library start
FetchContent_Declare(
  glaze
  GIT_REPOSITORY https://github.com/stephenberry/glaze.git
  GIT_TAG v1.5.5
  GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(glaze)
# glaze JSON library end

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

add_subdirectory("imgui")
# ImGui end

# Zydis start
FetchContent_Declare(zydis
    GIT_REPOSITORY git@github.com:zyantific/zydis.git
    GIT_TAG v4.0.0
    GIT_SHALLOW TRUE
    GIT_PROGRESS ON)

add_subdirectory("zydis")
# Zydis end

# safetyhook start
FetchContent_Declare(
    safetyhook
    GIT_REPOSITORY "https://github.com/UE4SS-RE/safetyhook.git"
    GIT_TAG "4def59079b95e08dc086cecf5515abde377059e5"
    GIT_PROGRESS ON)
FetchContent_MakeAvailable(safetyhook)

add_subdirectory("safetyhook")
# safetyhook end

# raw_pdb start
FetchContent_Declare(raw_pdb
    GIT_REPOSITORY git@github.com:MolecularMatters/raw_pdb.git
    GIT_TAG 8c6a7146393c83d27fa101e8bc8017f2a7f151df
    GIT_PROGRESS ON)

add_subdirectory("raw_pdb")
# raw_pdb end
# Third Party Depenedencies