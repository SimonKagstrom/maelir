#pragma once

#include "base_thread.hh"
#include "gps_port.hh"
#include "image.hh"
#include "tile.hh"

#include <atomic>
#include <etl/mutex.h>
#include <etl/queue_spsc_atomic.h>
#include <etl/vector.h>
#include <memory>
#include <vector>

constexpr auto kTileCacheSize = 4;
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
    TileProducer(std::unique_ptr<IGpsPort> gps_port);

    // Context: Another thread
    std::unique_ptr<ITileHandle> LockTile(uint32_t x, uint32_t y);

private:
    std::optional<milliseconds> OnActivation() final;

    std::unique_ptr<ImageImpl> DecodeTile(unsigned index);

    bool CacheTile(unsigned index);

    uint8_t EvictTile();

    etl::vector<std::unique_ptr<ImageImpl>, kTileCacheSize> m_tiles;
    std::atomic<uint32_t> m_locked_cache_entries {0};
    std::vector<uint8_t> m_tile_index_to_cache;

    etl::queue_spsc_atomic<uint32_t, kTileCacheSize> m_tile_requests;
    os::binary_semaphore m_tile_request_semaphore {0};

    std::unique_ptr<IGpsPort> m_gps_port;
    std::optional<GpsData> m_current_position;
    etl::mutex m_mutex;
};
