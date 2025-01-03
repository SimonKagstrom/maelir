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
    DecodeHelper(PNG& png, std::optional<uint16_t> mask_color, uint8_t* dst)
        : png(png)
        , mask_color(mask_color)
        , dst(dst)
        , offset(0)
        , line_buffer(std::make_unique<uint16_t[]>(png.getWidth()))
    {
    }

    DecodeHelper() = delete;

    PNG& png;
    std::optional<uint16_t> mask_color;
    uint8_t* dst;
    size_t offset;

    std::unique_ptr<uint16_t[]> line_buffer;
};

class StandaloneImage : public Image
{
public:
    StandaloneImage(std::unique_ptr<uint8_t[]> data, size_t width, size_t height, unsigned pixel_size)
        : Image(std::span<const uint8_t>({data.get(), width * height * pixel_size}), width, height, pixel_size != 2)
        , m_data(std::move(data))
    {
    }

private:
    std::unique_ptr<uint8_t[]> m_data;
};


void
PngDraw(PNGDRAW* pDraw)
{
    auto helper = static_cast<DecodeHelper*>(pDraw->pUser);

    helper->png.getLineAsRGB565(
        pDraw, helper->line_buffer.get(), PNG_RGB565_LITTLE_ENDIAN, 0xffffffff);

    if (helper->mask_color)
    {
        for (auto i = 0; i < pDraw->iWidth; i++)
        {
            auto pixel = helper->line_buffer[i];
            auto alpha_value = pixel == helper->mask_color ? LV_OPA_TRANSP : LV_OPA_COVER;

            auto b = pixel >> 11;
            auto g = (pixel >> 5) & 0x3f;
            auto r = pixel & 0x1f;

            // ARGB
            // RGB565 -> RGB888
            helper->dst[helper->offset++] = (r * 255) / 31;
            helper->dst[helper->offset++] = (g * 255) / 63;
            helper->dst[helper->offset++] = (b * 255) / 31;
            helper->dst[helper->offset++] = alpha_value;
        }
    }
    else
    {
        memcpy(helper->dst + helper->offset, helper->line_buffer.get(), pDraw->iWidth * 2);
        helper->offset += pDraw->iWidth * 2;
    }
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
    , m_tile_row_size(map_metadata.tile_row_size)
    , m_tile_rows(map_metadata.tile_rows)
{
    // Including the default land/empty tile
    assert(m_tile_count == m_tile_row_size * m_tile_rows + 1);

    m_tile_index_to_cache.resize(m_tile_count);

    std::ranges::fill(m_tile_index_to_cache, kInvalidTileIndex);
}


// Context: Another thread
std::unique_ptr<ITileHandle>
TileProducer::LockTile(const Point& point)
{
    auto index = PointToTileIndex(point);
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

bool
TileProducer::IsCached(const Point& point) const
{
    auto index = PointToTileIndex(point);
    if (!index)
    {
        return false;
    }

    std::scoped_lock lock(m_mutex);
    return m_tile_index_to_cache[*index] != kInvalidTileIndex;
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

    while (true)
    {
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
                if (m_locked_cache_entries & (1 << i))
                {
                    m_tile_request_order.push_back(index);
                    break;
                }

                m_tile_index_to_cache[tile->index] = kInvalidTileIndex;
                m_tiles[i] = nullptr;

                return i;
            }
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

    DecodeHelper priv(*png, std::nullopt, img->rgb565_data.data());

    rc = png->decode((void*)&priv, 0);
    png->close();
    if (rc != PNG_SUCCESS)
    {
        //printf("Argh tile %d @%p\n", index, flash_data);
        return nullptr;
    }

    return img;
}


std::optional<unsigned>
TileProducer::PointToTileIndex(const Point& point) const
{
    auto index = (point.y / kTileSize) * m_tile_row_size + point.x / kTileSize;
    if (index >= m_tile_count)
    {
        return std::nullopt;
    }

    return index;
}

std::unique_ptr<Image>
DecodePng(std::span<const uint8_t> data, std::optional<uint16_t> mask_color)
{
    auto png = std::make_unique<PNG>();

    auto rc = png->openFLASH((uint8_t*)data.data(), data.size(), PngDraw);

    if (rc != PNG_SUCCESS)
    {
        return nullptr;
    }

    auto pixel_size = mask_color ? 4 : 2;

    auto rgb565_data = std::make_unique<uint8_t[]>(png->getWidth() * png->getHeight() * pixel_size);

    DecodeHelper priv(*png, mask_color, rgb565_data.get());

    rc = png->decode((void*)&priv, 0);
    png->close();
    if (rc != PNG_SUCCESS)
    {
        return nullptr;
    }

    return std::make_unique<StandaloneImage>(
        std::move(rgb565_data), png->getWidth(), png->getHeight(), pixel_size);
}
