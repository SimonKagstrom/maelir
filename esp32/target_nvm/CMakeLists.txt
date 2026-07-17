add_library(target_nvm EXCLUDE_FROM_ALL
    target_nvm.cc
)

target_include_directories(target_nvm
PUBLIC
    include
)

target_link_libraries(target_nvm
PUBLIC
    idf::freertos
    idf::nvs_flash
    maelir_interface
)
