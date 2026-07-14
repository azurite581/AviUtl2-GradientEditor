cmake_minimum_required(VERSION 3.31)

if (NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 23)
endif()

if(NOT DEFINED PLUGIN_VERSION)
  set(PLUGIN_VERSION "0.1.0")
endif()

project(GradientEditor LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(IMGUI_SOURCE ${CMAKE_SOURCE_DIR}/third_party/imgui)
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# .cpp font file generation
include(${CMAKE_SOURCE_DIR}/src/fonts/ImGuiFontCodegen.cmake)
imgui_font_codegen(${IMGUI_SOURCE}/misc/fonts/binary_to_compressed_c.cpp)

# shader compilation
include(${CMAKE_SOURCE_DIR}/src/shaders/CompileShaders.cmake)

add_library(compiler_flags INTERFACE)
target_compile_features(compiler_flags INTERFACE cxx_std_${CMAKE_CXX_STANDARD})

add_library(${PROJECT_NAME} SHARED
  ${CMAKE_SOURCE_DIR}/src/core/app.cpp
  ${CMAKE_SOURCE_DIR}/src/core/d3d_manager.cpp
  ${CMAKE_SOURCE_DIR}/src/core/window_manager.cpp
  ${CMAKE_SOURCE_DIR}/src/core/script_bridge.cpp
  ${CMAKE_SOURCE_DIR}/src/fonts/material_symbols.cpp
  ${CMAKE_SOURCE_DIR}/src/ui/main_view.cpp
  ${CMAKE_SOURCE_DIR}/src/ui/widgets/gradient_data.cpp
  ${CMAKE_SOURCE_DIR}/src/ui/widgets/gradient_marker.cpp
  ${CMAKE_SOURCE_DIR}/src/ui/widgets/gradient_renderer.cpp
  ${CMAKE_SOURCE_DIR}/src/ui/widgets/gradient_widget.cpp
  ${CMAKE_SOURCE_DIR}/src/ui/widgets/preset_window.cpp
  ${CMAKE_SOURCE_DIR}/src/ui/widgets/history_window.cpp
  ${CMAKE_SOURCE_DIR}/src/main.cpp
  ${CMAKE_SOURCE_DIR}/src/utils/aviutl2/logger_wrapper_interface.cpp
  ${CMAKE_SOURCE_DIR}/src/utils/aviutl2/config2_wrapper_interface.cpp
  ${GENERATED_CPP}
  ${COMPILED_PIXEL_SHADER}
  ${COMPILED_VERTEX_SHADER}
)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS NO)
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".aux2")

# macro definition
target_compile_definitions(${PROJECT_NAME} PRIVATE
  MARKER_COUNT=${MARKER_COUNT}
  PLUGIN_VERSION="${PLUGIN_VERSION}"
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
target_include_directories(imgui PUBLIC
  ${IMGUI_SOURCE}
  ${CMAKE_SOURCE_DIR}/src  # for config/imconfg.h
)
target_compile_definitions(imgui PUBLIC
  IMGUI_USER_CONFIG="config/imconfig.h"
)
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

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src/core)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src/fonts)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src/shaders)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src/ui)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src/ui/widgets)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src/utils/aviutl2)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src/utils/common)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src/utils/imgui)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src/utils/grd_codec)

# AviUtl2 SDK
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/third_party/aviutl2_sdk_mirror/include)

# nlohmann json
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/third_party/json/single_include/nlohmann)

# IconFont
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/third_party/IconFontCppHeaders)

if(WIN32)
  target_compile_definitions(${PROJECT_NAME} PRIVATE
  UNICODE
  _UNICODE

  $<$<CONFIG:Debug>:_DEBUG>
  $<$<CONFIG:Release>:NDEBUG>
)
endif()

# Compiler Options ------------------------------------------------------------------
if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE
    /utf-8
    /W4
    /sdl

    /permissive-
    $<$<CONFIG:Debug>:/Zi>

    $<IF:$<CONFIG:Debug>,/Od,/O2>
    $<$<CONFIG:Release>:/Oi>

    $<IF:$<CONFIG:Debug>,/GL-,/GL>
    $<IF:$<CONFIG:Debug>,/Gy-,/Gy>
    /EHsc
  )
  target_link_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Release>:/LTCG>

    $<$<CONFIG:Release>:/OPT:REF>
    $<$<CONFIG:Release>:/OPT:ICF>

    # Hybrid CRT
    $<$<CONFIG:Debug>:/NODEFAULTLIB:libucrtd.lib>
    $<$<CONFIG:Debug>:/DEFAULTLIB:ucrtd.lib>
    $<$<NOT:$<CONFIG:Debug>>:/NODEFAULTLIB:libucrt.lib>
    $<$<NOT:$<CONFIG:Debug>>:/DEFAULTLIB:ucrt.lib>
  )
endif()

# copy compile_commands.json to source directory for clangd
if(CMAKE_EXPORT_COMPILE_COMMANDS)
  add_custom_target(copy_compile_commands ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${CMAKE_BINARY_DIR}/compile_commands.json
      ${CMAKE_SOURCE_DIR}/compile_commands.json
    DEPENDS ${CMAKE_BINARY_DIR}/compile_commands.json
  )
endif()
