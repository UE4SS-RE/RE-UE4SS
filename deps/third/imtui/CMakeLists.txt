include(FetchContent)
FetchContent_MakeAvailable(imtui)
FetchContent_MakeAvailable(ImGuiTextEdit)
FetchContent_GetProperties(imtui SOURCE_DIR ImTui_FETCHED_SOURCE)
FetchContent_GetProperties(ImGuiTextEdit SOURCE_DIR ImGuiTextEdit_FETCHED_SOURCE)

message(STATUS "ImTui_FETCHED_SOURCE: ${ImTui_FETCHED_SOURCE}")

set(ImGuiText_Sources
    "${ImGuiTextEdit_FETCHED_SOURCE}/TextEditor.cpp"
)

add_library(ImGuiText ${ImGuiText_Sources})

target_link_libraries(ImGuiText
    PUBLIC
        imtui
)

target_include_directories(ImGuiText PUBLIC
    "${ImTui_FETCHED_SOURCE}/third-party/imgui/imgui/"
    "${ImGuiTextEdit_FETCHED_SOURCE}"
)
