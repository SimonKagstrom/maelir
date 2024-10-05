add_library(application_state
    application_state.cc
)

target_link_libraries(application_state
PUBLIC
    maelir_interface
)

target_include_directories(application_state
PUBLIC
    include
)
