#include "tile_producer.hh"

#include <fmt/format.h>
#include <png.h>

namespace
{

class TileHandle : public ITileHandle
{
public:
    explicit TileHandle(const Image& image)
        : m_image(image)
    {
    }

    ~TileHandle() final
    {
    }

    const Image& GetImage() const final
    {
        return m_image;
    }

private:
    const Image& m_image;
};

std::optional<unsigned>
PointToTileIndex(uint32_t x, uint32_t y)
{
    constexpr auto row = 11;

    // TMP!
    if (x > 480)
    {
        return std::nullopt;
    }

    if (y > 480)
    {
        return std::nullopt;
    }

    return (y / 240) * row + x / 240;
}

} // namespace


// Context: Another thread
std::unique_ptr<ITileHandle>
TileProducer::LockTile(uint32_t x, uint32_t y)
{
    auto index = PointToTileIndex(x, y);
    if (index)
    {
        auto tile = DecodeTile(*index);
        if (tile)
        {
            m_tiles.push_back(std::move(*tile));
            return std::make_unique<TileHandle>(m_tiles.back());
        }
    }

    return nullptr;
}

std::optional<milliseconds>
TileProducer::OnActivation()
{
    return std::nullopt;
}

std::unique_ptr<TileProducer::ImageImpl>
TileProducer::DecodeTile(unsigned index)
{
    auto fn = fmt::format("/Users/ska/projects/maelir/tmp/tile00{:02}.png", index);

    FILE* fp = fopen(fn.c_str(), "r");

    png_structp pngptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop pnginfo = png_create_info_struct(pngptr);

    png_init_io(pngptr, fp);
    png_set_palette_to_rgb(pngptr);


    png_bytepp rows;
    png_read_png(pngptr, pnginfo, PNG_TRANSFORM_IDENTITY, NULL);
    rows = png_get_rows(pngptr, pnginfo);

    auto w = png_get_image_width(pngptr, pnginfo);
    auto h = png_get_image_height(pngptr, pnginfo);

    auto img = std::make_unique<ImageImpl>();
    img->rgb565_data.resize(w * h);
    auto p = img->rgb565_data.data();

    for (auto i = 0; i < h; i++)
    {
        for (auto j = 0; j < h * 3; j += 3)
        {
            auto r = rows[i][j];
            auto g = rows[i][j + 1];
            auto b = rows[i][j + 2];

            auto rgb565 = __builtin_bswap16((r >> 3) << 11 | (g >> 2) << 5 | (b >> 3));

            *p = rgb565;
            p++;
        }
    }

    img->height = h;
    img->width = w;
    img->data = img->rgb565_data;

    return img;
}
