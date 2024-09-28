#pragma once

#include "base_thread.hh"
#include "gps_port.hh"
#include "hal/i_display.hh"
#include "route_service.hh"
#include "tile_producer.hh"

class UserInterface : public os::BaseThread
{
public:
    UserInterface(const MapMetadata& metadata,
                  TileProducer& tile_producer,
                  hal::IDisplay& display,
                  std::unique_ptr<IGpsPort> gps_port,
                  std::unique_ptr<IRouteListener> route_listener);

private:
    struct TileAndPosition
    {
        std::unique_ptr<ITileHandle> tile;
        Point position;
    };
    std::optional<milliseconds> OnActivation() final;

    bool NeedsRedraw(int32_t x, int32_t y) const;

    void RequestMapTiles();

    void DrawMap();
    void DrawRoute();
    void DrawBoat();

    PixelPosition PositionToMapCenter(const auto& pixel_position) const;


    const uint32_t m_tile_rows;
    const uint32_t m_tile_columns;
    const uint32_t m_land_mask_rows;
    const uint32_t m_land_mask_row_size;

    TileProducer& m_tile_producer;

    hal::IDisplay& m_display;
    std::unique_ptr<IGpsPort> m_gps_port;
    std::unique_ptr<IRouteListener> m_route_listener;

    // Global pixel position of the boat
    int32_t m_x {0};
    int32_t m_y {0};

    // Global pixel position of the left corner of the map
    int32_t m_map_x {0};
    int32_t m_map_y {0};
    std::span<const IndexType> m_current_route;

    etl::vector<TileAndPosition, kTileCacheSize> m_tiles;

    uint16_t* m_frame_buffer {nullptr};
    std::unique_ptr<Image> m_boat;
    std::unique_ptr<uint16_t[]> m_boat_rotation_buffer;
    std::span<uint16_t> m_boat_rotation;

    Image m_rotated_boat;
};
