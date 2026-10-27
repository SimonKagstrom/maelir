add_library(maelir_messages INTERFACE)

target_include_directories(maelir_messages
INTERFACE
    include
)

target_link_libraries(maelir_messages
INTERFACE
    base_messages
    post_office
)
