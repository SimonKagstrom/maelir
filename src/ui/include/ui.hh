#pragma once

#include "base_thread.hh"
#include "gps_port.hh"
#include "hal/i_display.hh"
#include "tile_producer.hh"

class UserInterface : public os::BaseThread
{
public:
    UserInterface(TileProducer& tile_producer,
                  hal::IDisplay& display,
                  std::unique_ptr<IGpsPort> gps_port);

private:
    std::optional<milliseconds> OnActivation() final;

    TileProducer& m_tile_producer;

    hal::IDisplay& m_display;
    std::unique_ptr<IGpsPort> m_gps_port;

    // TMP!
    int32_t m_x {0};
    int32_t m_y {0};
};
