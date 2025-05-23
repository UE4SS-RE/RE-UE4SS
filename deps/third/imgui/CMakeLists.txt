include(FetchContent)

project(ImGui)

set(FETCHCONTENT_QUIET OFF)

# Check if content is already populated
FetchContent_GetProperties(ImGui)
if(NOT imgui_POPULATED)
    FetchContent_MakeAvailable(ImGui)
endif()
FetchContent_GetProperties(ImGui SOURCE_DIR ImGui_SOURCE_DIR)

FetchContent_GetProperties(ImGuiTextEdit)
if(NOT imguitextedit_POPULATED)
    FetchContent_MakeAvailable(ImGuiTextEdit)
endif()
FetchContent_GetProperties(ImGuiTextEdit SOURCE_DIR ImGuiTextEdit_SOURCE_DIR)

FetchContent_GetProperties(IconFontCppHeaders)
if(NOT iconfontcppheaders_POPULATED)
    FetchContent_MakeAvailable(IconFontCppHeaders)
endif()
FetchContent_GetProperties(IconFontCppHeaders SOURCE_DIR IconFontCppHeaders_SOURCE_DIR)

set(ImGui_Sources
    "${ImGui_SOURCE_DIR}/imgui.cpp"
    "${ImGui_SOURCE_DIR}/imgui_draw.cpp"
    "${ImGui_SOURCE_DIR}/imgui_tables.cpp"
    "${ImGui_SOURCE_DIR}/imgui_widgets.cpp"
    "${ImGui_SOURCE_DIR}/imgui_demo.cpp"
    "${ImGui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp"
    "${ImGui_SOURCE_DIR}/backends/imgui_impl_win32.cpp"
    "${ImGui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp"
    "${ImGui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp"
    "${ImGui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp"
    "${ImGuiTextEdit_SOURCE_DIR}/TextEditor.cpp")

add_library(ImGui ${ImGui_Sources})

# Enabling c++23 support
target_compile_features(ImGui PUBLIC cxx_std_23)

# Fix offsetof macro issue with Windows SDK
# if(CMAKE_SYSTEM_NAME STREQUAL "Windows" AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # # Comment out the problematic static asserts by disabling them
    # target_compile_options(ImGui PRIVATE -Wno-invalid-offsetof)
    # target_compile_definitions(ImGui PRIVATE 
        # _CRT_USE_BUILTIN_OFFSETOF
    # )
# endif()

target_link_libraries(ImGui PUBLIC glfw)

target_include_directories(ImGui PUBLIC 
    "${ImGui_SOURCE_DIR}"
    "${ImGuiTextEdit_SOURCE_DIR}"
    "${IconFontCppHeaders_SOURCE_DIR}")

# Configure ImGui based on target platform and compiler
# Check if we're cross-compiling from Linux to Windows
if(CMAKE_SYSTEM_NAME STREQUAL "Windows" AND CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    # Cross-compiling - disable SSE to avoid intrinsics linking issues
    message(WARNING "Configuring ImGui without SSE support (cross-compilation from Linux to Windows)")
    target_compile_definitions(ImGui PRIVATE IMGUI_DISABLE_SSE)
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
    # Native builds or other configurations can use SSE
    message(STATUS "Configuring ImGui with SSE support")
    target_compile_options(ImGui PRIVATE -msse -msse2)
else()
    # Non-x86_64 architectures
    message(STATUS "Configuring ImGui without SSE support (non-x86_64 architecture)")
    target_compile_definitions(ImGui PRIVATE IMGUI_DISABLE_SSE)
endif()

# Suppress warnings from ImGui
# Uses suppress_current_third_party_warnings() from cmake/modules/ThirdPartyWarnings.cmake
suppress_current_third_party_warnings()
