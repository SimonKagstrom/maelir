add_library(storage EXCLUDE_FROM_ALL
    storage.cc
)

target_include_directories(storage
PUBLIC
    include
)

target_link_libraries(storage
PUBLIC
    maelir_interface
    application_state
)
