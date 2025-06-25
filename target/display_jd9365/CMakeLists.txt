add_library(display_jd9365 EXCLUDE_FROM_ALL
    display_jd9365.cc
)

target_include_directories(display_jd9365
PUBLIC
    include
)

target_link_libraries(display_jd9365
PUBLIC
    idf::freertos
    idf::esp_lcd
    idf::esp_timer
    idf::espressif__esp_lcd_jd9365
    maelir_interface
)
