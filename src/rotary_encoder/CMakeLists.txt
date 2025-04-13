add_library(rotary_encoder EXCLUDE_FROM_ALL
    rotary_encoder.cc
)

target_include_directories(rotary_encoder
PUBLIC
    include
)

target_link_libraries(rotary_encoder
PUBLIC
    button_debouncer
)
