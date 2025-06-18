include (cmake/bin_to_source.cmake)

add_subdirectory(external)
add_subdirectory(src)

add_custom_target(generate_git_version
  ${CMAKE_COMMAND} -D SOURCE_DIRECTORY=${CMAKE_CURRENT_SOURCE_DIR}
                   -D DST_FILE=${CMAKE_BINARY_DIR}/include/version.hh
                   -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/generate_version.cmake
  DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/.git/HEAD
)

add_library(git_version_header INTERFACE)

target_include_directories(git_version_header
INTERFACE
    ${CMAKE_BINARY_DIR}/include
)

add_dependencies(git_version_header
    generate_git_version
)
