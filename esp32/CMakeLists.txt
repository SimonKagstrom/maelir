add_compile_options(-Wdouble-promotion)

add_subdirectory(.. maelir)
add_subdirectory(../external/libmaelir/esp32 libmaelir_esp32)

add_library(generated_application_state ALIAS maelir_application_state)
