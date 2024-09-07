if (NOT "${IDF_TARGET}" STREQUAL "esp32")
    # Build as shared for host, since this otherwise conflicts with libz
    set(LIB_TYPE SHARED)
else()
    set(LIB_TYPE STATIC)
endif()

add_library(pngdec_zlib ${LIB_TYPE} EXCLUDE_FROM_ALL
    pngdec/src/adler32.c
    pngdec/src/crc32.c
    pngdec/src/infback.c
    pngdec/src/inffast.c
    pngdec/src/inflate.c
    pngdec/src/inftrees.c
    pngdec/src/inftrees.c
    pngdec/src/zutil.c
)

# Add here, since pngdec doesn't have a CMakeLists.txt
add_library(pngdec ${LIB_TYPE} EXCLUDE_FROM_ALL
    pngdec/src/PNGdec.cpp
)
target_link_libraries(pngdec
PRIVATE
    # Link privately, to avoid conflicts with system libz
    pngdec_zlib
)

target_compile_options(pngdec
PRIVATE
    -Wno-deprecated-non-prototype
)

target_compile_definitions(pngdec
PUBLIC
    -D__MACH__
)

target_include_directories(pngdec PUBLIC
    pngdec/src/
)
