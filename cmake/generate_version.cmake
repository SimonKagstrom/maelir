# From kcov / https://www.marcusfolkesson.se/blog/git-version-in-cmake/
set(GIT_VERSION "<unknown>")
if (EXISTS "${SOURCE_DIRECTORY}/.git")
    find_program (GIT_EXECUTABLE NAMES git REQUIRED)

    execute_process (COMMAND "${GIT_EXECUTABLE}"
            "--git-dir=${SOURCE_DIRECTORY}/.git"
            describe
            --always
            --abbrev=8
            --tags
            HEAD
        OUTPUT_VARIABLE GIT_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()
message (STATUS "maelir version: ${GIT_VERSION}")

configure_file(${SOURCE_DIRECTORY}/cmake/version.hh.in ${DST_FILE} @ONLY)
