# Add here, since pngdec doesn't have a CMakeLists.txt
add_library(pngdec EXCLUDE_FROM_ALL
    pngdec/src/adler32.c
    pngdec/src/crc32.c
    pngdec/src/infback.c
    pngdec/src/inffast.c
    pngdec/src/inflate.c
    pngdec/src/inftrees.c
    pngdec/src/inftrees.c
    pngdec/src/PNGdec.cpp
    pngdec/src/zutil.c
)

target_compile_options(pngdec PRIVATE
    -Wno-deprecated-non-prototype
)

target_include_directories(pngdec PUBLIC
    pngdec/src/
)
