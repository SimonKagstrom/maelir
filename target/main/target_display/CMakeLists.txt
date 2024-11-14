add_library(target_display EXCLUDE_FROM_ALL
    target_display.cc
    PCA9554.c
    MyUtils.c
    MyWire.c
)

target_include_directories(target_display
PUBLIC
    include
)

target_link_libraries(target_display
PUBLIC
    idf::freertos
    idf::esp_lcd
    idf::esp_timer
    maelir_interface
)
