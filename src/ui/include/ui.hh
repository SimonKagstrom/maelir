#pragma once

#include "base_thread.hh"
#include "gps_port.hh"
#include "hal/i_display.hh"
#include "hal/i_input.hh"
#include "route_service.hh"
#include "tile_producer.hh"

#include <etl/queue_spsc_atomic.h>

class UserInterface : public os::BaseThread, public hal::IInput::IListener
{
public:
    UserInterface(const MapMetadata& metadata,
                  TileProducer& tile_producer,
                  hal::IDisplay& display,
                  hal::IInput& input,
                  std::unique_ptr<IGpsPort> gps_port,
                  std::unique_ptr<IRouteListener> route_listener);

private:
    enum class State
    {
        kMap,
        kZoomedOutMap,
        kGlobalMap,

        kValueCount,
    };

    struct TileAndPosition
    {
        std::unique_ptr<ITileHandle> tile;
        Point position;
    };
    std::optional<milliseconds> OnActivation() final;

    void OnInput(const hal::IInput::Event& event) final;

    bool NeedsRedraw(int32_t x, int32_t y) const;

    void RequestMapTiles();

    void DrawMap();
    void DrawRoute();
    void DrawBoat();
    void DrawSpeedometer();

    Point PositionToMapCenter(const auto& pixel_position) const;


    const uint32_t m_tile_rows;
    const uint32_t m_tile_columns;
    const uint32_t m_land_mask_rows;
    const uint32_t m_land_mask_row_size;

    TileProducer& m_tile_producer;

    hal::IDisplay& m_display;
    std::unique_ptr<IGpsPort> m_gps_port;
    std::unique_ptr<IRouteListener> m_route_listener;

    // Global pixel position of the boat
    Point m_position {0, 0};

    // Global pixel position of the left corner of the map
    Point m_map_position {0, 0};

    // Current speed in knots
    float m_speed {0};

    std::vector<IndexType> m_route;
    std::optional<unsigned> m_passed_route_index;

    etl::queue_spsc_atomic<hal::IInput::Event, 4> m_input_queue;

    etl::vector<TileAndPosition, kTileCacheSize> m_tiles;

    uint16_t* m_frame_buffer {nullptr};
    std::unique_ptr<Image> m_boat;
    std::unique_ptr<Image> m_speed_dial;
    std::unique_ptr<Image> m_speedometer;
    std::vector<uint16_t> m_boat_rotation;

    Image m_rotated_boat;

    State m_state {State::kMap};
    bool m_show_speedometer {true};
};
