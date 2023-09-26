include(FetchContent)

project(ImGui)

set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(ImGui
    GIT_REPOSITORY git@github.com:ocornut/imgui.git
    GIT_TAG v1.89
    GIT_PROGRESS ON
    CONFIGURE_COMMAND "")

FetchContent_Populate(ImGui)
FetchContent_GetProperties(ImGui SOURCE_DIR ImGui_FETCHED_SOURCE)

FetchContent_Declare(ImGuiTextEdit
    GIT_REPOSITORY git@github.com:UE4SS-RE/ImGuiColorTextEdit.git
    GIT_TAG master
    GIT_PROGRESS ON
    CONFIGURE_COMMAND "")

FetchContent_Populate(ImGuiTextEdit)
FetchContent_GetProperties(ImGuiTextEdit SOURCE_DIR ImGuiTextEdit_FETCHED_SOURCE)

set(ImGui_Sources
    "${ImGui_FETCHED_SOURCE}/imgui.cpp"
    "${ImGui_FETCHED_SOURCE}/imgui_draw.cpp"
    "${ImGui_FETCHED_SOURCE}/imgui_tables.cpp"
    "${ImGui_FETCHED_SOURCE}/imgui_widgets.cpp"
    "${ImGui_FETCHED_SOURCE}/misc/cpp/imgui_stdlib.cpp"
    "${ImGui_FETCHED_SOURCE}/backends/imgui_impl_win32.cpp"
    "${ImGui_FETCHED_SOURCE}/backends/imgui_impl_dx11.cpp"
    "${ImGui_FETCHED_SOURCE}/backends/imgui_impl_opengl3.cpp"
    "${ImGui_FETCHED_SOURCE}/backends/imgui_impl_glfw.cpp"
    "${ImGuiTextEdit_FETCHED_SOURCE}/TextEditor.cpp")

add_library(ImGui ${ImGui_Sources})

# Enabling c++20 support
target_compile_features(ImGui PUBLIC cxx_std_20)

target_link_libraries(ImGui PUBLIC GLFW)

target_include_directories(ImGui PUBLIC 
    "${ImGui_FETCHED_SOURCE}"
    "${ImGuiTextEdit_FETCHED_SOURCE}")
