#pragma once

#include "base_thread.hh"
#include "gps_port.hh"
#include "image.hh"
#include "tile.hh"

#include <etl/mutex.h>
#include <etl/queue_spsc_atomic.h>
#include <etl/vector.h>
#include <memory>
#include <vector>

constexpr auto kTileCacheSize = 16;

class ITileHandle
{
public:
    virtual ~ITileHandle() = default;

    virtual const Image& GetImage() const = 0;
};

struct ImageImpl : public Image
{
    ~ImageImpl()
    {
        free(rgb565_data);
    }

    unsigned int index;
    uint16_t *rgb565_data;
//    std::array<uint16_t, kTileSize * kTileSize> rgb565_data;
};

class TileProducer : public os::BaseThread
{
public:
    TileProducer(std::unique_ptr<IGpsPort> gps_port);

    // Context: Another thread
    std::unique_ptr<ITileHandle> LockTile(uint32_t x, uint32_t y);

//private:
    std::optional<milliseconds> OnActivation() final;

    std::unique_ptr<ImageImpl> DecodeTile(unsigned index);

    etl::vector<std::unique_ptr<ImageImpl>, kTileCacheSize> m_tiles;
    std::vector<uint8_t> m_tile_index_to_cache;

    etl::queue_spsc_atomic<uint32_t, kTileCacheSize> m_tile_requests;
    os::binary_semaphore m_tile_request_semaphore {0};

    std::unique_ptr<IGpsPort> m_gps_port;
    etl::mutex m_mutex;
};
