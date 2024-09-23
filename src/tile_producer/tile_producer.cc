#include "tile_producer.hh"

#include "generated_tiles.hh"
#include "hal/i_display.hh"
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

TileProducer::TileProducer()
{
    m_tile_index_to_cache.resize(kRowSize * kColumnSize);

    std::ranges::fill(m_tile_index_to_cache, kInvalidTileIndex);
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
            if (m_tile_index_to_cache[*index] == kInvalidTileIndex)
            {
                m_mutex.unlock();
                return nullptr;
            }
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
    uint32_t requested_index = 0;

    while (m_tile_requests.pop(requested_index))
    {
        CacheTile(requested_index);
        m_tile_request_semaphore.release();
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
        assert(m_tiles[cache_index] == nullptr);
        m_tiles[cache_index] = std::move(tile);
    }
    else
    {
        m_tiles.push_back(std::move(tile));
    }
    m_tile_request_order.push_back(requested_index);

    m_tile_index_to_cache[requested_index] = cache_index;

    return true;
}

uint8_t
TileProducer::EvictTile()
{
    if (m_tile_request_order.empty())
    {
        assert(false);
        return 0;
    }

    auto index = m_tile_request_order.front();
    m_tile_request_order.pop_front();

    for (auto i = 0; i < m_tiles.size(); i++)
    {
        auto& tile = m_tiles[i];
        if (!tile)
        {
            // Shouldn't be possible, but anyway
            continue;
        }
        if (tile->index == index)
        {
            m_tile_index_to_cache[tile->index] = kInvalidTileIndex;
            m_tiles[i] = nullptr;

            return i;
        }
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
    auto img = std::make_unique<ImageImpl>(index);

    DecodeHelper priv {*png, img->rgb565_data.data(), 0};

    rc = png->decode((void*)&priv, 0);
    png->close();
    if (rc != PNG_SUCCESS)
    {
        printf("Argh tile %d @%p\n", index, src.data());
        return nullptr;
    }

    return img;
}
