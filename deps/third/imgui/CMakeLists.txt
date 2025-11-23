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

# Backend options - automatically enable based on platform
option(IMGUI_BACKEND_WIN32 "Enable Win32 backend" ${WIN32})
option(IMGUI_BACKEND_DX11 "Enable DirectX 11 backend" ${WIN32})
option(IMGUI_BACKEND_OPENGL3 "Enable OpenGL 3 backend" ON)
option(IMGUI_BACKEND_GLFW "Enable GLFW backend" ON)

# Core ImGui sources (always included)
set(ImGui_Core_Sources
    "${ImGui_SOURCE_DIR}/imgui.cpp"
    "${ImGui_SOURCE_DIR}/imgui_draw.cpp"
    "${ImGui_SOURCE_DIR}/imgui_tables.cpp"
    "${ImGui_SOURCE_DIR}/imgui_widgets.cpp"
    "${ImGui_SOURCE_DIR}/imgui_demo.cpp"
    "${ImGui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp"
    "${ImGuiTextEdit_SOURCE_DIR}/TextEditor.cpp")

# Platform-specific backend sources
set(ImGui_Backend_Sources)

# Win32 backend (Windows only)
if(IMGUI_BACKEND_WIN32 AND WIN32)
    list(APPEND ImGui_Backend_Sources "${ImGui_SOURCE_DIR}/backends/imgui_impl_win32.cpp")
    message(STATUS "ImGui: Enabled Win32 backend")
endif()

# DirectX 11 backend (Windows only)
if(IMGUI_BACKEND_DX11 AND WIN32)
    list(APPEND ImGui_Backend_Sources "${ImGui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp")
    message(STATUS "ImGui: Enabled DirectX 11 backend")
endif()

# OpenGL 3 backend (cross-platform)
if(IMGUI_BACKEND_OPENGL3)
    list(APPEND ImGui_Backend_Sources "${ImGui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp")
    message(STATUS "ImGui: Enabled OpenGL 3 backend")
endif()

# GLFW backend (cross-platform)
if(IMGUI_BACKEND_GLFW)
    list(APPEND ImGui_Backend_Sources "${ImGui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp")
    message(STATUS "ImGui: Enabled GLFW backend")
endif()

# Create the library with core + selected backends
add_library(ImGui ${ImGui_Core_Sources} ${ImGui_Backend_Sources})

# Enabling c++23 support
target_compile_features(ImGui PUBLIC cxx_std_23)

# Platform-specific compile definitions
if(WIN32)
    target_compile_definitions(ImGui PUBLIC IMGUI_IMPL_WIN32_DISABLE_GAMEPAD)
endif()

# Handle platform-specific linking
set(ImGui_Link_Libraries)

# GLFW is required only if GLFW backend is enabled
if(IMGUI_BACKEND_GLFW)
    if(TARGET glfw)
        list(APPEND ImGui_Link_Libraries glfw)
    else()
        message(WARNING "ImGui: GLFW backend enabled but glfw target not found")
    endif()
endif()

# DirectX 11 libraries (Windows only)
if(IMGUI_BACKEND_DX11 AND WIN32)
    list(APPEND ImGui_Link_Libraries d3d11 d3dcompiler dxgi)
endif()

# OpenGL libraries
if(IMGUI_BACKEND_OPENGL3)
    if(WIN32)
        list(APPEND ImGui_Link_Libraries opengl32)
    elseif(APPLE)
        find_package(OpenGL REQUIRED)
        list(APPEND ImGui_Link_Libraries ${OPENGL_LIBRARIES})
    else()
        find_package(OpenGL REQUIRED)
        list(APPEND ImGui_Link_Libraries ${OPENGL_LIBRARIES})
    endif()
endif()

# Link the libraries
if(ImGui_Link_Libraries)
    target_link_libraries(ImGui PUBLIC ${ImGui_Link_Libraries})
endif()

# Include directories
target_include_directories(ImGui PUBLIC 
    "${ImGui_SOURCE_DIR}"
    "${ImGuiTextEdit_SOURCE_DIR}"
    "${IconFontCppHeaders_SOURCE_DIR}")

# Suppress warnings from ImGui using the project's system
suppress_current_third_party_warnings()