include_directories(lvgl_setup)
add_compile_definitions(LV_CONF_INCLUDE_SIMPLE=1)

find_package(etl REQUIRED)

add_subdirectory(.. maelir)
add_subdirectory(target_display)
add_subdirectory(target_gps)
add_subdirectory(target_input)
add_subdirectory(target_nvm)
add_subdirectory(os)
