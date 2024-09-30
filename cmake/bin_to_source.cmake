function(library_from_data DATA_FILE BASE_NAME)
    set(DEST_LIBRARY ${BASE_NAME}_library)
    set(OUTPUT_DIR ${CMAKE_BINARY_DIR}/${BASE_NAME}_generated/)
    set(out_hh ${OUTPUT_DIR}/${BASE_NAME}.hh)
    set(out_cc ${OUTPUT_DIR}/${BASE_NAME}.cc)

    add_custom_command(
        OUTPUT ${out_cc} ${out_hh}
        COMMAND mkdir -p ${OUTPUT_DIR}
        COMMAND
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../tools/bin_to_source.py
            ${OUTPUT_DIR}
            ${DATA_FILE}
            ${BASE_NAME}

        DEPENDS ${DATA_FILE}
        DEPENDS ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../tools/bin_to_source.py
    )

    add_custom_target(${DEST_LIBRARY}_target
        DEPENDS ${out_tile_hpp}
        DEPENDS ${out_land_cpp} ${out_land_hpp}
    )

    # Library for the C++/header files
    add_library(${DEST_LIBRARY} STATIC EXCLUDE_FROM_ALL
        ${out_hh}
        ${out_cc}
    )

    # The address sanitizer is redundant for this (generated) data
    target_compile_options(${DEST_LIBRARY}
    PRIVATE
        -fno-sanitize=all
    )
    add_dependencies(${DEST_LIBRARY} ${DEST_LIBRARY}_target)

    target_include_directories(${DEST_LIBRARY}
    PUBLIC
        ${OUTPUT_DIR}
    )
endfunction()
