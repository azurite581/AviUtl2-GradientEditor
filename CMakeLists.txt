cmake_minimum_required(VERSION 3.31)

if (NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 23)
endif()

project(GradientEditor LANGUAGES CXX)

set(IMGUI_SOURCE third_party/imgui)
set(MARKER_COUNT 30 CACHE STRING "marker count")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# .cpp font file generation
include(${CMAKE_SOURCE_DIR}/src/fonts/ImGuiFontCodegen.cmake)

# shader compilation
include(${CMAKE_SOURCE_DIR}/src/shaders/CompileShaders.cmake)

add_library(compiler_flags INTERFACE)
target_compile_features(compiler_flags INTERFACE cxx_std_${CMAKE_CXX_STANDARD})

add_library(${PROJECT_NAME} SHARED
    src/core/app.cpp
    src/core/app_state.cpp
    src/core/d3d_manager.cpp
    src/core/window_manager.cpp
    src/core/script_bridge.cpp
    src/fonts/material_symbols.cpp
    src/ui/main_view.cpp
    src/ui/widgets/gradient_data.cpp
    src/ui/widgets/gradient_marker.cpp
    src/ui/widgets/gradient_renderer.cpp
    src/ui/widgets/gradient_widget.cpp
    src/ui/widgets/preset_window.cpp
    src/main.cpp
    src/utils/aviutl2/logger_wrapper_interface.cpp
    src/utils/aviutl2/config2_wrapper_interface.cpp
    ${GENERATED_CPP}
    ${COMPILED_PIXEL_SHADER}
    ${COMPILED_VERTEX_SHADER}
)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS NO)
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".aux2")

# macro definition
target_compile_definitions(${PROJECT_NAME} PRIVATE
    MARKER_COUNT=${MARKER_COUNT}
)

target_link_libraries(${PROJECT_NAME} PRIVATE compiler_flags)

# Libraries -------------------------------------------------------------------------
# ImGui
add_library(imgui STATIC
    ${IMGUI_SOURCE}/imgui.cpp
    ${IMGUI_SOURCE}/imgui_demo.cpp
    ${IMGUI_SOURCE}/imgui_draw.cpp
    ${IMGUI_SOURCE}/imgui_tables.cpp
    ${IMGUI_SOURCE}/imgui_widgets.cpp
    ${IMGUI_SOURCE}/misc/cpp/imgui_stdlib.cpp
)
target_include_directories(imgui PUBLIC ${IMGUI_SOURCE})
add_library(imgui_dx11 STATIC
    ${IMGUI_SOURCE}/backends/imgui_impl_dx11.cpp
    ${IMGUI_SOURCE}/backends/imgui_impl_win32.cpp
)
target_include_directories(imgui_dx11 PUBLIC ${IMGUI_SOURCE}/backends)
target_link_libraries(imgui_dx11 PUBLIC
    imgui
    d3d11
    dxgi
)
target_link_libraries(${PROJECT_NAME} PRIVATE imgui_dx11)

target_include_directories(${PROJECT_NAME} PRIVATE src)
target_include_directories(${PROJECT_NAME} PRIVATE src/core)
target_include_directories(${PROJECT_NAME} PRIVATE src/fonts)
target_include_directories(${PROJECT_NAME} PRIVATE src/shaders)
target_include_directories(${PROJECT_NAME} PRIVATE src/ui)
target_include_directories(${PROJECT_NAME} PRIVATE src/ui/widgets)
target_include_directories(${PROJECT_NAME} PRIVATE src/utils/aviutl2)
target_include_directories(${PROJECT_NAME} PRIVATE src/utils/common)
target_include_directories(${PROJECT_NAME} PRIVATE src/utils/imgui)

# AviUtl2 SDK
target_include_directories(${PROJECT_NAME} PRIVATE third_party/aviutl2_sdk_mirror/include)

# nlohmann json
target_include_directories(${PROJECT_NAME} PRIVATE third_party/json/single_include/nlohmann)

# IconFont
target_include_directories(${PROJECT_NAME} PRIVATE third_party/IconFontCppHeaders)

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE _UNICODE)
endif()

# Compiler Options ------------------------------------------------------------------
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
    target_compile_options(${PROJECT_NAME} PRIVATE
        /source-charset:utf-8
        /W4
        $<IF:$<CONFIG:Debug>,/Od,/O2>
        $<IF:$<CONFIG:Debug>,/GL-,/GL>
        $<IF:$<CONFIG:Debug>,/Gy-,/Gy>
        $<$<CONFIG:Debug>:/Zi>
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:/LTCG>
        # Hybrid CRT
        /NODEFAULTLIB:$<IF:$<CONFIG:Debug>,libucrtd.lib,libucrt.lib>
        /DEFAULTLIB:$<IF:$<CONFIG:Debug>,ucrtd.lib,ucrt.lib>
    )
endif()

