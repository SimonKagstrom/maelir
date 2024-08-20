#include "tile_producer.hh"

#include "tile.hh"
#include "tile_utils.hh"

#include <PNGdec.h>
#include <mutex>


constexpr auto kInvalidTileIndex = kTileCacheSize;

namespace
{

struct ReadHelper
{
    const std::byte* data;
    size_t offset;
};

struct DecodeHelper
{
    PNG& png;
    uint16_t* dst;
    size_t offset;
};

void
PngDraw(PNGDRAW* pDraw)
{
    auto helper = static_cast<DecodeHelper*>(pDraw->pUser);

    helper->png.getLineAsRGB565(
        pDraw, helper->dst + helper->offset, PNG_RGB565_LITTLE_ENDIAN, 0xffffffff);
    helper->offset += pDraw->iWidth;
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
    : BaseThread(0)
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
    auto png = std::make_unique<PNG>();

    auto& src = tile_array[index];
    auto rc = png->openFLASH((uint8_t*)src.data(), src.size(), PngDraw);

    if (rc != PNG_SUCCESS)
    {
        return nullptr;
    }
    auto img = std::make_unique<ImageImpl>();

    img->index = index;
    img->height = png->getHeight();
    img->width = png->getWidth();
    img->rgb565_data = (uint16_t*)malloc(kTileSize * kTileSize * 2);
    img->data = std::span<const uint16_t> {img->rgb565_data, kTileSize * kTileSize};

    DecodeHelper priv {*png, img->rgb565_data, 0};

    rc = png->decode((void*)&priv, 0);
    png->close();
    if (rc != PNG_SUCCESS)
    {
        return nullptr;
    }

    return img;
}
