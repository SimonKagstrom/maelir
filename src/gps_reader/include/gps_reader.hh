#pragma once

#include "base_thread.hh"
#include "gps_port.hh"
#include "hal/i_gps.hh"
#include "tile.hh"

#include <array>
#include <atomic>
#include <etl/vector.h>

class GpsReader : public os::BaseThread
{
public:
    explicit GpsReader(const MapMetadata& metadata, hal::IGps& gps);

    std::unique_ptr<IGpsPort> AttachListener();

private:
    class GpsPortImpl;

    std::optional<milliseconds> OnActivation() final;

    void Reset();

    hal::IGps& m_gps;

    // A copy of the map metadata (to place in PSRAM)
    const MapMetadata &m_map_metadata;

    etl::vector<GpsPortImpl*, 8> m_listeners;
    std::array<std::atomic_bool, 8> m_stale_listeners;

    std::optional<GpsPosition> m_position;
    std::optional<float> m_speed;
    std::optional<float> m_heading;
};
