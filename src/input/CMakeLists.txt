add_library(encoder_input EXCLUDE_FROM_ALL
    encoder_input.cc
)

target_include_directories(encoder_input
PUBLIC
    include
)

target_link_libraries(encoder_input
PUBLIC
    rotary_encoder
    button_debouncer
)
