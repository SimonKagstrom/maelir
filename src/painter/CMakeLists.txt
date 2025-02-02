add_library(painter EXCLUDE_FROM_ALL
    painter.cc
)

target_link_libraries(painter
PUBLIC
    maelir_interface
    lvgl
)

target_include_directories(painter
PUBLIC
    include
)
