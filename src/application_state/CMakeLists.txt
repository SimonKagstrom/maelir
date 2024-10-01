add_library(application_state INTERFACE)

target_link_libraries(application_state INTERFACE
    maelir_interface
)

target_include_directories(application_state INTERFACE
    include
)