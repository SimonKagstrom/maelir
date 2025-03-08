find_package(etl REQUIRED)

add_compile_options(-Wdouble-promotion)

add_subdirectory(.. maelir)
add_subdirectory(target_display)
add_subdirectory(target_gps)
add_subdirectory(target_input)
add_subdirectory(target_nvm)
add_subdirectory(os)
