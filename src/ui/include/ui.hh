#pragma once

#include "base_thread.hh"
#include "tile_producer.hh"

class Ui : public os::BaseThread
{
public:
    Ui(TileProducer& tile_producer);

private:
    std::optional<milliseconds> OnActivation() final;

    TileProducer& m_tile_producer;

    // TMP!
    uint32_t m_x {0};
    uint32_t m_y {0};
};
