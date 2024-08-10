function(tilegen DEST_LIBRARY IMAGE)
    set(OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/${DEST_LIBRARY})
    set(out_cpp ${OUTPUT_DIR}/tile.cc)
    set(out_hpp ${OUTPUT_DIR}/tile.hh)

    add_custom_command(
        OUTPUT ${out_cpp} ${out_hpp}
        COMMAND mkdir -p ${OUTPUT_DIR}
        COMMAND
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../tools/tiler.py
            ${IMAGE}
            ${OUTPUT_DIR}
            tile

        DEPENDS ${IMAGE}
        DEPENDS ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../tools/tiler.py
    )

    add_custom_target(${DEST_LIBRARY}_target
        DEPENDS ${out_cpp} ${out_hpp}
    )

    # Library for the C++/header files
    add_library(${DEST_LIBRARY} STATIC EXCLUDE_FROM_ALL
        ${out_cpp} ${out_hpp}
    )
    add_dependencies(${DEST_LIBRARY} ${DEST_LIBRARY}_target)

    target_include_directories(${DEST_LIBRARY}
    PUBLIC
        ${OUTPUT_DIR}
    )
endfunction()
