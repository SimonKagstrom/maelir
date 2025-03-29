add_library(event_serializer EXCLUDE_FROM_ALL
    event_serializer.cc
)

target_include_directories(event_serializer
PUBLIC
    include
)

target_link_libraries(event_serializer
PUBLIC
    maelir_interface
)
