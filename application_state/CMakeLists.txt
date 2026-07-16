set(TOP ${CMAKE_CURRENT_LIST_DIR}/../)
include(${TOP}/external/libmaelir/cmake/generate_application_state.cmake)

set(MAELIR_APPLICATION_STATE
    "${TOP}/application_state/parameters.yml"
    # TODO: Should come from libmaelir
    "${TOP}/external/libmaelir/application_state/parameters_common.yml"
    "${TOP}/external/libmaelir/application_state/application_state_common.yml"
    "${TOP}/application_state/application_state.yml"
)
# Export for parent directories (for example test/unittest) that add this project as a subdirectory.
set(MAELIR_APPLICATION_STATE
    "${MAELIR_APPLICATION_STATE}"
    CACHE INTERNAL "MAELIR application-state YAML inputs")
list(JOIN MAELIR_APPLICATION_STATE ";" MAELIR_APPLICATION_STATE_STR)

generate_application_state(maelir_application_state "${MAELIR_APPLICATION_STATE_STR}")


target_link_libraries(maelir_application_state
PUBLIC
    libmaelir_application_state
    maelir_interface
)

target_include_directories(maelir_application_state
PUBLIC
    include
)
