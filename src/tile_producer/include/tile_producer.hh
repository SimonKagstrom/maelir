#pragma once

#include "base_thread.hh"
#include "hal/i_display.hh"
#include "image.hh"
#include "tile.hh"

#include <atomic>
#include <etl/list.h>
#include <etl/mutex.h>
#include <etl/queue_spsc_atomic.h>
#include <etl/vector.h>
#include <memory>
#include <vector>

// Cache all visible tiles, plus a few for good measure
constexpr auto kTileCacheSize =
    2 + ((hal::kDisplayWidth / kTileSize) + 1) * ((hal::kDisplayHeight / kTileSize) + 1);
static_assert(kTileCacheSize <= 32); // For the uint32_t atomic

class ITileHandle
{
public:
    virtual ~ITileHandle() = default;

    virtual const Image& GetImage() const = 0;
};

class ImageImpl : public Image
{
public:
    ImageImpl(unsigned index)
        : Image(std::span<const uint16_t>(rgb565_data), kTileSize, kTileSize)
        , index(index)
    {
    }

    ~ImageImpl()
    {
    }

    unsigned int index;
    std::array<uint16_t, kTileSize * kTileSize> rgb565_data;
};

class TileProducer : public os::BaseThread
{
public:
    TileProducer(const MapMetadata& flash_tile_data);

    // Context: Another thread
    std::unique_ptr<ITileHandle> LockTile(const Point& point);

    bool IsCached(const Point& point) const;

private:
    std::optional<milliseconds> OnActivation() final;

    std::unique_ptr<ImageImpl> DecodeTile(unsigned index);

    bool CacheTile(unsigned index);

    uint8_t EvictTile();
    std::optional<unsigned> PointToTileIndex(const Point& point) const;

    const uint8_t* m_flash_start;
    const FlashTile* m_flash_tile_data;
    const uint32_t m_tile_count;
    const uint32_t m_tile_row_size;
    const uint32_t m_tile_rows;

    etl::vector<std::unique_ptr<ImageImpl>, kTileCacheSize> m_tiles;
    etl::list<uint32_t, kTileCacheSize> m_tile_request_order;
    std::atomic<uint32_t> m_locked_cache_entries {0};
    std::vector<uint8_t> m_tile_index_to_cache;

    etl::queue_spsc_atomic<uint32_t, kTileCacheSize> m_tile_requests;
    os::binary_semaphore m_tile_request_semaphore {0};

    mutable etl::mutex m_mutex;
};

std::unique_ptr<Image> DecodePng(std::span<const uint8_t> data);
