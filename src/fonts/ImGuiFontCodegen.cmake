add_executable(binary_to_compressed_c ${IMGUI_SOURCE}/misc/fonts/binary_to_compressed_c.cpp)

set(SOURCE_FONT_FILE "${CMAKE_CURRENT_LIST_DIR}/MaterialSymbolsOutlined[FILL,GRAD,opsz,wght].ttf")
set(GENERATED_CPP_TMP "${CMAKE_CURRENT_LIST_DIR}/material_symbols.cpp.tmp")
set(GENERATED_CPP "${CMAKE_CURRENT_LIST_DIR}/material_symbols.cpp")

add_custom_command(
    OUTPUT ${GENERATED_CPP_TMP}
    COMMAND binary_to_compressed_c
            "${SOURCE_FONT_FILE}"
            material_symbols
            > "${GENERATED_CPP_TMP}"
    DEPENDS binary_to_compressed_c "${SOURCE_FONT_FILE}"
    CODEGEN
)

add_custom_command(
    OUTPUT "${GENERATED_CPP}"
    COMMAND powershell -NoProfile -Command
        "\"Get-Content '${GENERATED_CPP_TMP}' | Select-Object -Skip 2 | Set-Content '${GENERATED_CPP}'\""
    DEPENDS "${GENERATED_CPP_TMP}"
)
