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

    PixelPosition PositionToPoint(const GpsPosition& gps_data) const;

    void Reset();

    hal::IGps& m_gps;
    // From the metadata
    const double m_corner_latitude;
    const double m_corner_longitude;
    const uint32_t m_latitude_pixel_size;
    const uint32_t m_longitude_pixel_size;
    const uint32_t m_tile_columns;
    const uint32_t m_tile_rows;

    etl::vector<GpsPortImpl*, 8> m_listeners;
    std::array<std::atomic_bool, 8> m_stale_listeners;

    std::optional<GpsPosition> m_position;
    std::optional<float> m_speed;
    std::optional<float> m_heading;
};
