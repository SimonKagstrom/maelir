add_library(target_display EXCLUDE_FROM_ALL
    target_display.cc
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
    idf::esp_lcd_st7701
    idf::esp_io_expander_tca9554
    idf::espressif__esp_lcd_panel_io_additions
    maelir_interface
)
