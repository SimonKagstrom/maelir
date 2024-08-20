#include "tile_producer.hh"

#include "hal/i_display.hh"
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
    explicit TileHandle(ImageImpl& image,
                        uint8_t cache_index,
                        std::atomic<uint32_t>& locked_cache_entries)
        : m_image(image)
        , m_cache_index(cache_index)
        , m_locked_cache_entries(locked_cache_entries)
    {
        m_locked_cache_entries |= (1 << m_cache_index);
    }

    ~TileHandle() final
    {
        // Unlock the cache entry
        m_locked_cache_entries &= ~(1 << m_cache_index);
    }

    const Image& GetImage() const final
    {
        return m_image;
    }

private:
    ImageImpl& m_image;
    const uint8_t m_cache_index;
    std::atomic<uint32_t>& m_locked_cache_entries;
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
    auto index = PointToTileIndex(x, y);
    if (index)
    {
        m_mutex.lock();

        while (m_tile_index_to_cache[*index] == kInvalidTileIndex)
        {
            m_tile_requests.push(*index);

            // Release the lock while waiting for the producer thread
            m_mutex.unlock();
            Awake();
            m_tile_request_semaphore.acquire();

            m_mutex.lock();
        }

        auto cache_index = m_tile_index_to_cache[*index];
        auto out = std::make_unique<TileHandle>(
            *m_tiles[cache_index], cache_index, m_locked_cache_entries);
        m_mutex.unlock();

        return std::move(out);
    }

    return nullptr;
}

std::optional<milliseconds>
TileProducer::OnActivation()
{
    auto gps_data = m_gps_port->Poll();
    if (gps_data)
    {
        // Make sure these tiles are decoded
        m_current_position = gps_data;
    }

    uint32_t requested_index = 0;
    while (m_tile_requests.pop(requested_index))
    {
        if (CacheTile(requested_index))
        {
            m_tile_request_semaphore.release();
        }
    }

    return std::nullopt;
}

bool
TileProducer::CacheTile(unsigned requested_index)
{
    auto tile = DecodeTile(requested_index);
    if (!tile)
    {
        return false;
    }

    std::scoped_lock lock(m_mutex);
    auto cache_index = m_tiles.size();
    if (m_tiles.full())
    {
        cache_index = EvictTile();
        m_tiles[cache_index] = std::move(tile);
    }
    else
    {
        m_tiles.push_back(std::move(tile));
    }

    m_tile_index_to_cache[requested_index] = cache_index;

    return true;
}

uint8_t
TileProducer::EvictTile()
{
    if (m_current_position)
    {
        auto [x, y] = PositionToPointCenteredAndClamped(*m_current_position);

        auto center_tile = PointToTileIndex(x, y);
        assert(center_tile);

        auto center_tile_x = *center_tile % kRowSize;
        auto center_tile_y = *center_tile / kRowSize;

        for (auto i = 0; i < m_tiles.size(); i++)
        {
            auto& tile = m_tiles[i];
            if (!tile)
            {
                // Shouldn't be possible, but anyway
                continue;
            }

            if (m_locked_cache_entries & (1 << i))
            {
                // Tile is locked, can't evict
                continue;
            }

            auto tile_x = tile->index % kRowSize;
            auto tile_y = tile->index / kRowSize;

            // Not one of the displayed tiles? Evict!
            if (tile_x < center_tile_x - 2 || tile_x > center_tile_x + 2 ||
                tile_y < center_tile_y - 2 || tile_y > center_tile_y + 2)
            {
                m_tile_index_to_cache[tile->index] = kInvalidTileIndex;
                return tile->index;
            }
        }
    }

    for (auto i = 0; i < m_tiles.size(); i++)
    {
        auto& tile = m_tiles[i];
        if (!tile)
        {
            // Shouldn't be possible, but anyway
            continue;
        }

        if (m_locked_cache_entries & (1 << i))
        {
            // Tile is locked, can't evict
            continue;
        }

        m_tile_index_to_cache[tile->index] = kInvalidTileIndex;
        return tile->index;
    }

    assert(false);
    return 0;
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
