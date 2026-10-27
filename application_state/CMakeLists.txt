set(TOP ${CMAKE_CURRENT_LIST_DIR}/../)
include(${TOP}/external/libmaelir/cmake/generate_application_state.cmake)

generate_application_state(maelir_application_state
    BASE
        libmaelir_application_state
    FILES
        parameters.yml
        application_state.yml
)

target_link_libraries(maelir_application_state
PUBLIC
    maelir_interface
)

target_include_directories(maelir_application_state
PUBLIC
    include
)
