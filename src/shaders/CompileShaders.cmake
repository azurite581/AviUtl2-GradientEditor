# fxc.exe の検索
find_program(FXC_PATH NAMES fxc.exe
    HINTS
        "$ENV{ProgramFiles\(x86\)}/Windows Kits/10/Bin/10.0.26100.0/x64"
)

if (NOT FXC_PATH)
    message(FATAL_ERROR "fxc.exe not found. Please install Windows SDK.")
endif()

message(STATUS "Found fxc.exe at: ${FXC_PATH}")

set(COMPILED_PIXEL_SHADER "${CMAKE_CURRENT_LIST_DIR}/pixel_shader.h")
set(COMPILED_VERTEX_SHADER "${CMAKE_CURRENT_LIST_DIR}/vertex_shader.h")

# ピクセルシェーダーのコンパイル設定
add_custom_command(
  OUTPUT ${COMPILED_PIXEL_SHADER}
  COMMAND ${FXC_PATH}
          /T ps_5_0
          /E psmain
          /Fh "${COMPILED_PIXEL_SHADER}"
          "${CMAKE_CURRENT_LIST_DIR}/pixel_shader.hlsl"
          /nologo
          /D MARKER_COUNT=${MARKER_COUNT}
  DEPENDS "${CMAKE_CURRENT_LIST_DIR}/pixel_shader.hlsl"
  COMMENT "Compiling Pixel Shader: pixel_shader.hlsl"
  VERBATIM
)

# 頂点シェーダーのコンパイル設定
add_custom_command(
  OUTPUT ${COMPILED_VERTEX_SHADER}
  COMMAND ${FXC_PATH}
          /T vs_5_0
          /E vsmain
          /Fh "${COMPILED_VERTEX_SHADER}"
          "${CMAKE_CURRENT_LIST_DIR}/vertex_shader.hlsl"
          /nologo
  DEPENDS "${CMAKE_CURRENT_LIST_DIR}/vertex_shader.hlsl"
  COMMENT "Compiling Vertex Shader: vertex_shader.hlsl"
  VERBATIM
)

add_custom_target(CompileShaders ALL
  DEPENDS ${COMPILED_PIXEL_SHADER} ${COMPILED_VERTEX_SHADER}
)

