add_library(target_gpio EXCLUDE_FROM_ALL
    target_gpio.cc
)

target_include_directories(target_gpio
PUBLIC
    include
)

target_link_libraries(target_gpio
PUBLIC
    idf::driver
    maelir_interface
)
