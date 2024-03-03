include(FetchContent)

project(ImGui)

set(FETCHCONTENT_QUIET OFF)

FetchContent_Populate(ImGui)
FetchContent_GetProperties(ImGui SOURCE_DIR ImGui_FETCHED_SOURCE)

FetchContent_Populate(ImGuiTextEdit)
FetchContent_GetProperties(ImGuiTextEdit SOURCE_DIR ImGuiTextEdit_FETCHED_SOURCE)

FetchContent_Populate(IconFontCppHeaders)
FetchContent_GetProperties(IconFontCppHeaders SOURCE_DIR IconFontCppHeaders_FETCHED_SOURCE)

set(ImGui_Sources
    "${ImGui_FETCHED_SOURCE}/imgui.cpp"
    "${ImGui_FETCHED_SOURCE}/imgui_draw.cpp"
    "${ImGui_FETCHED_SOURCE}/imgui_tables.cpp"
    "${ImGui_FETCHED_SOURCE}/imgui_widgets.cpp"
    "${ImGui_FETCHED_SOURCE}/imgui_demo.cpp"
    "${ImGui_FETCHED_SOURCE}/misc/cpp/imgui_stdlib.cpp"
    "${ImGuiTextEdit_FETCHED_SOURCE}/TextEditor.cpp")

set(ImGui_Win32_Sources
    "${ImGui_FETCHED_SOURCE}/backends/imgui_impl_opengl3.cpp"
    "${ImGui_FETCHED_SOURCE}/backends/imgui_impl_glfw.cpp"
    "${ImGui_FETCHED_SOURCE}/backends/imgui_impl_win32.cpp"
    "${ImGui_FETCHED_SOURCE}/backends/imgui_impl_dx11.cpp")


set(ImGui_Linux_Sources
    "${ImGui_FETCHED_SOURCE}/backends/imgui_impl_opengl3.cpp"
    "${ImGui_FETCHED_SOURCE}/backends/imgui_impl_glfw.cpp")


if(WIN32)
    list(APPEND ImGui_Sources ${ImGui_Win32_Sources})
elseif(LINUX)
    list(APPEND ImGui_Sources ${ImGui_Linux_Sources})
endif()

add_library(ImGui ${ImGui_Sources})

# Enabling c++20 support
target_compile_features(ImGui PUBLIC cxx_std_20)

target_link_libraries(ImGui PUBLIC GLFW)

target_include_directories(ImGui PUBLIC 
    "${ImGui_FETCHED_SOURCE}"
    "${ImGuiTextEdit_FETCHED_SOURCE}"
    "${IconFontCppHeaders_FETCHED_SOURCE}")
