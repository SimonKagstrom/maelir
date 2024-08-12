#include "tile_producer.hh"

#include "tile.hh"
#include "tile_utils.hh"

#include <fmt/format.h>
#include <mutex>
#include <png.h>

constexpr auto kInvalidTileIndex = kTileCacheSize;

namespace
{

struct ReadHelper
{
    const std::byte* data;
    size_t offset;
};

void
readpng(png_structp _pngptr, png_bytep _data, png_size_t _len)
{
    /* Get input */
    auto input = static_cast<ReadHelper*>(png_get_io_ptr(_pngptr));

    /* Copy data from input */
    memcpy(_data, reinterpret_cast<const char*>(input->data) + input->offset, _len);
    input->offset += _len;
}

class TileHandle : public ITileHandle
{
public:
    explicit TileHandle(ImageImpl& image)
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
    ImageImpl& m_image;
};

} // namespace

TileProducer::TileProducer(std::unique_ptr<IGpsPort> gps_port)
    : BaseThread()
    , m_gps_port(std::move(gps_port))
{
    m_tile_index_to_cache.resize(kRowSize * kColumnSize);
    std::ranges::fill(m_tile_index_to_cache, kInvalidTileIndex);

    m_gps_port->AwakeOn(GetSemaphore());
}


// Context: Another thread
std::unique_ptr<ITileHandle>
TileProducer::LockTile(uint32_t x, uint32_t y)
{
    std::scoped_lock lock(m_mutex);

    auto index = PointToTileIndex(x, y);
    if (index)
    {
        while (m_tile_index_to_cache[*index] == kInvalidTileIndex)
        {
            m_tile_requests.push(*index);
            Awake();
            m_tile_request_semaphore.acquire();
        }

        return std::make_unique<TileHandle>(*m_tiles[m_tile_index_to_cache[*index]]);
    }

    return nullptr;
}

std::optional<milliseconds>
TileProducer::OnActivation()
{
    uint32_t requested_index = 0;

    while (m_tile_requests.pop(requested_index))
    {
        auto tile = DecodeTile(requested_index);
        if (tile)
        {
            auto cache_index = m_tiles.size();

            m_tiles.push_back(std::move(tile));
            m_tile_index_to_cache[requested_index] = cache_index;

            m_tile_request_semaphore.release();
        }
    }

    auto gps_data = m_gps_port->Poll();
    if (gps_data)
    {
        // Make sure these tiles are decoded
    }

    return std::nullopt;
}

std::unique_ptr<ImageImpl>
TileProducer::DecodeTile(unsigned index)
{
    png_structp pngptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop pnginfo = png_create_info_struct(pngptr);

    ReadHelper helper {tile_array[index].data(), 0};

    png_set_read_fn(pngptr, (png_voidp)&helper, readpng);
    png_set_palette_to_rgb(pngptr);

    png_bytepp rows;
    png_read_png(pngptr, pnginfo, PNG_TRANSFORM_IDENTITY, NULL);
    rows = png_get_rows(pngptr, pnginfo);

    auto w = png_get_image_width(pngptr, pnginfo);
    auto h = png_get_image_height(pngptr, pnginfo);

    auto img = std::make_unique<ImageImpl>();
    img->index = index;
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
