#pragma once

#include "application_state.hh"
#include "base_thread.hh"
#include "gps_port.hh"
#include "hal/i_display.hh"
#include "hal/i_input.hh"
#include "route_service.hh"
#include "tile_producer.hh"

#include <etl/queue_spsc_atomic.h>
#include <etl/vector.h>
#include <lvgl.h>

class ScreenBase
{
public:
    ScreenBase()
        : m_screen(lv_obj_create(nullptr))
    {
    }

    virtual ~ScreenBase() = default;

    virtual void Activate() = 0;

    virtual void Update() = 0;

    virtual void OnPosition(const GpsData& position)
    {
    }

protected:
    lv_obj_t *m_screen;
};



class UserInterface : public os::BaseThread, public hal::IInput::IListener
{
public:
    UserInterface(ApplicationState& application_state,
                  const MapMetadata& metadata,
                  TileProducer& tile_producer,
                  hal::IDisplay& display,
                  hal::IInput& input,
                  std::unique_ptr<IGpsPort> gps_port,
                  std::unique_ptr<IRouteListener> route_listener);

private:
    class MapScreen;
    class MenuScreen;

    enum class Mode
    {
        kMap,
        kZoom2,
        kZoom4,

        kValueCount,
    };

    // From BaseThread
    void OnStartup() final;
    std::optional<milliseconds> OnActivation() final;

    void OnInput(const hal::IInput::Event& event) final;


    const uint32_t m_tile_rows;
    const uint32_t m_tile_row_size;
    const uint32_t m_land_mask_rows;
    const uint32_t m_land_mask_row_size;

    ApplicationState& m_application_state;
    TileProducer& m_tile_producer;

    hal::IDisplay& m_display;
    lv_display_t* m_lvgl_display {nullptr};
    std::unique_ptr<IGpsPort> m_gps_port;
    std::unique_ptr<IRouteListener> m_route_listener;

    // Global pixel position of the boat
    Point m_position {0, 0};

    // Current speed in knots
    float m_speed {0};

    std::vector<IndexType> m_route;
    std::optional<unsigned> m_passed_route_index;

    etl::queue_spsc_atomic<hal::IInput::Event, 4> m_input_queue;

    std::unique_ptr<ScreenBase> m_map_screen;
    std::unique_ptr<ScreenBase> m_menu_screen;

    etl::vector<Point, ((hal::kDisplayWidth * hal::kDisplayHeight) / kTileSize) * 4>
        m_zoomed_out_map_tiles;

    int32_t m_zoom_level {1};
    Mode m_mode {Mode::kMap};
    bool m_show_speedometer {true};

    std::unique_ptr<os::ITimer> m_button_timer;
};
