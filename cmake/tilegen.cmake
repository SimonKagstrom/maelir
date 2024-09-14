function(tilegen DEST_LIBRARY YAML_FILE)
    set(OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/${DEST_LIBRARY})
    set(out_tile_cpp ${OUTPUT_DIR}/generated_tiles.cc)
    set(out_tile_hpp ${OUTPUT_DIR}/generated_tiles.hh)
    set(out_land_cpp ${OUTPUT_DIR}/generated_land_mask.cc)
    set(out_land_hpp ${OUTPUT_DIR}/generated_land_mask.hh)

    add_custom_command(
        OUTPUT ${out_tile_cpp} ${out_tile_hpp} ${out_land_cpp} ${out_land_hpp}
        COMMAND mkdir -p ${OUTPUT_DIR}
        COMMAND
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../tools/tiler.py
            ${YAML_FILE}
            ${OUTPUT_DIR}
            generated_tiles
            generated_land_mask

        DEPENDS ${YAML_FILE}
        DEPENDS ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../tools/tiler.py
    )

    add_custom_target(${DEST_LIBRARY}_target
        DEPENDS ${out_tile_cpp} ${out_tile_hpp}
        DEPENDS ${out_land_cpp} ${out_land_hpp}
    )

    # Library for the C++/header files
    add_library(${DEST_LIBRARY} STATIC EXCLUDE_FROM_ALL
        ${out_tile_cpp} ${out_tile_hpp}
        ${out_land_cpp} ${out_land_hpp}
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
