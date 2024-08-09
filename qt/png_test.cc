#include <array>
#include <png.h>

#include "tile.hh"

void
readpng(png_structp _pngptr, png_bytep _data, png_size_t _len)
{
    static int offset = 0;

    /* Get input */
    auto input = png_get_io_ptr(_pngptr);

    /* Copy data from input */
    memcpy(_data, static_cast<const char*>(input) + offset, _len);
    offset += _len;
}

int
main(int argc, const char* argv[])
{
    //    FILE* fp = fopen(argv[1], "r");

    png_structp pngptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop pnginfo = png_create_info_struct(pngptr);

    png_set_read_fn(pngptr, (png_voidp)(tile.data()), readpng);
    png_set_palette_to_rgb(pngptr);


    png_bytepp rows;
    png_read_png(pngptr, pnginfo, PNG_TRANSFORM_IDENTITY, NULL);
    rows = png_get_rows(pngptr, pnginfo);

    for (auto i = 0; i < png_get_image_height(pngptr, pnginfo); i++)
    {
        for (auto j = 0; j < png_get_image_width(pngptr, pnginfo) * 3; j += 3)
        {
            printf("%d %d %d ", rows[i][j], rows[i][j + 1], rows[i][j + 2]);
        }
        printf("\n");
    }

    png_destroy_read_struct(&pngptr, &pnginfo, nullptr);
    png_destroy_info_struct(pngptr, &pnginfo);

    return 0;
}
