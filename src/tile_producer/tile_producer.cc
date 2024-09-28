#include "tile_producer.hh"

#include "hal/i_display.hh"

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

class StandaloneImage : public Image
{
public:
    StandaloneImage(std::unique_ptr<uint16_t[]> data, size_t width, size_t height)
        : Image(std::span<const uint16_t>({data.get(), width * height}), width, height)
        , m_data(std::move(data))
    {
    }

private:
    std::unique_ptr<uint16_t[]> m_data;
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

TileProducer::TileProducer(const MapMetadata& map_metadata)
    : m_flash_start(reinterpret_cast<const uint8_t*>(&map_metadata))
    , m_flash_tile_data(
          reinterpret_cast<const FlashTile*>(m_flash_start + map_metadata.tile_data_offset))
    , m_tile_count(map_metadata.tile_count)
    , m_tile_rows(map_metadata.tile_row_size)
    , m_tile_columns(map_metadata.tile_column_size)
{
    m_tile_index_to_cache.resize(map_metadata.tile_row_size * map_metadata.tile_column_size);

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
    if (index >= m_tile_count)
    {
        return nullptr;
    }

    auto png = std::make_unique<PNG>();

    const auto& tile = m_flash_tile_data[index];
    const auto flash_data = m_flash_start + tile.flash_offset;
    const auto tile_size = tile.size;

    auto in_psram = std::make_unique<uint8_t[]>(tile_size);
    memcpy(in_psram.get(), flash_data, tile_size);

    auto rc = png->openFLASH(in_psram.get(), tile_size, PngDraw);

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
        printf("Argh tile %d @%p\n", index, flash_data);
        return nullptr;
    }

    return img;
}


std::optional<unsigned>
TileProducer::PointToTileIndex(uint32_t x, uint32_t y) const
{
    if (x >= kTileSize * m_tile_rows)
    {
        return std::nullopt;
    }

    if (y >= kTileSize * m_tile_columns)
    {
        return std::nullopt;
    }

    return (y / kTileSize) * m_tile_rows + x / kTileSize;
}

std::unique_ptr<Image>
DecodePng(std::span<const uint8_t> data)
{
    auto png = std::make_unique<PNG>();

    auto rc = png->openFLASH((uint8_t*)data.data(), data.size(), PngDraw);

    if (rc != PNG_SUCCESS)
    {
        return nullptr;
    }

    auto rgb565_data = std::make_unique<uint16_t[]>(png->getWidth() * png->getHeight() * 2);

    DecodeHelper priv {*png, rgb565_data.get(), 0};

    rc = png->decode((void*)&priv, 0);
    png->close();
    if (rc != PNG_SUCCESS)
    {
        return nullptr;
    }

    return std::make_unique<StandaloneImage>(
        std::move(rgb565_data), png->getWidth(), png->getHeight());
}
