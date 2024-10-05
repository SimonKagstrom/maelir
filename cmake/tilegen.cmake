function(tilegen YAML_FILE)
    set(OUTPUT_DIR ${CMAKE_BINARY_DIR})
    set(out_bin ${OUTPUT_DIR}/map_data.bin)

    add_custom_command(
        OUTPUT ${out_bin}
        COMMAND mkdir -p ${OUTPUT_DIR}
        COMMAND
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../tools/tiler.py
            ${YAML_FILE}
            ${OUTPUT_DIR}

        DEPENDS ${YAML_FILE}
        DEPENDS ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../tools/tiler.py
    )

    add_custom_target(generated_map_data
        DEPENDS ${out_bin}
    )
endfunction()
