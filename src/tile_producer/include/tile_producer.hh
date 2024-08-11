#pragma once

#include "base_thread.hh"
#include "image.hh"

#include <etl/mutex.h>
#include <etl/vector.h>
#include <memory>
#include <vector>

constexpr auto kTileCacheSize = 8;

class ITileHandle
{
public:
    virtual ~ITileHandle() = default;

    virtual const Image& GetImage() const = 0;
};

struct ImageImpl : public Image
{
    unsigned int index;
    std::vector<uint16_t> rgb565_data;
};

class TileProducer : public os::BaseThread
{
public:
    TileProducer();

    // Context: Another thread
    std::unique_ptr<ITileHandle> LockTile(uint32_t x, uint32_t y);

private:
    std::unique_ptr<ImageImpl> DecodeTile(unsigned index);

    std::optional<milliseconds> OnActivation() final;

    etl::vector<std::unique_ptr<ImageImpl>, kTileCacheSize> m_tiles;
    std::vector<uint8_t> m_tile_index_to_cache;

    etl::mutex m_mutex;
};
