#pragma once

#include "base_thread.hh"
#include "image.hh"

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

class TileProducer : public os::BaseThread
{
public:
    std::unique_ptr<ITileHandle> LockTile(uint32_t x, uint32_t y);

private:
    struct ImageImpl : public Image
    {
        std::vector<uint16_t> rgb565_data;
    };

    std::unique_ptr<ImageImpl> DecodeTile(unsigned index);

    std::optional<milliseconds> OnActivation() final;

    etl::vector<ImageImpl, kTileCacheSize> m_tiles;
};
