find_package(etl REQUIRED)

add_compile_options(-Wdouble-promotion)

add_subdirectory(.. maelir)
add_subdirectory(../external/libmaelir/esp32 libmaelir_esp32)
