#pragma once

#include "application_state.hh"
#include "base_thread.hh"
#include "gps_port.hh"
#include "hal/i_display.hh"
#include "hal/i_input.hh"
#include "ota_updater.hh"
#include "route_service.hh"
#include "tile_producer.hh"
#include "timer_manager.hh"

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

    virtual ~ScreenBase()
    {
        lv_obj_delete(m_screen);
    }

    virtual void Activate()
    {
        lv_screen_load(m_screen);
    }

    virtual void OnPosition(const GpsData& position)
    {
    }

    virtual void OnInput(hal::IInput::Event event)
    {
    }

    virtual void Update()
    {
    }

protected:
    lv_obj_t* m_screen;
};


class UserInterface : public os::BaseThread, public hal::IInput::IListener
{
public:
    UserInterface(ApplicationState& application_state,
                  const MapMetadata& metadata,
                  TileProducer& tile_producer,
                  hal::IDisplay& display,
                  hal::IInput& input,
                  RouteService& route_service,
                  OtaUpdater& ota_updater,
                  std::unique_ptr<IGpsPort> gps_port,
                  std::unique_ptr<IRouteListener> route_listener);

private:
    enum class PositionSelection
    {
        kHome,
        kNewRoute,
    };

    class MapScreen;
    class MenuScreen;
    class UpdatingScreen;

    // From BaseThread
    void OnStartup() final;
    std::optional<milliseconds> OnActivation() final;

    void OnInput(const hal::IInput::Event& event) final;


    static void StaticLvglEncoderRead(lv_indev_t* indev, lv_indev_data_t* data);
    static void
    StaticLvglFlushCallback(lv_display_t* display, const lv_area_t* area, uint8_t* px_map);


    void EnterMenu();

    void EnterOtaUpdatingScreen();

    void SelectPosition(PositionSelection selection);

    const uint32_t m_tile_rows;
    const uint32_t m_tile_row_size;
    const uint32_t m_land_mask_rows;
    const uint32_t m_land_mask_row_size;

    ApplicationState& m_application_state;
    TileProducer& m_tile_producer;

    hal::IDisplay& m_display;
    hal::IInput& m_input;
    RouteService& m_route_service;
    OtaUpdater& m_ota_updater;

    lv_display_t* m_lvgl_display {nullptr};
    lv_indev_t* m_lvgl_input_dev {nullptr};

    std::unique_ptr<IGpsPort> m_gps_port;
    std::unique_ptr<IRouteListener> m_route_listener;

    // Global pixel position of the boat
    Point m_position {0, 0};

    // Current speed in knots
    float m_speed {0};

    // Icon states
    bool m_calculating_route {false};
    bool m_gps_position_valid {false};
    os::TimerHandle m_gps_position_timer;
    os::TimerHandle m_updated_timer;


    std::vector<IndexType> m_route;
    int m_passed_route_index {-1};

    etl::queue_spsc_atomic<hal::IInput::Event, 4> m_input_queue;

    // The map is always active, but the menu can be created/deleted
    std::unique_ptr<ScreenBase> m_map_screen;
    std::unique_ptr<ScreenBase> m_menu_screen;
    std::unique_ptr<ScreenBase> m_updating_screen;

    int16_t m_enc_diff {0};
    lv_indev_state_t m_button_state {LV_INDEV_STATE_RELEASED};

    // Position selection data
    std::optional<UserInterface::PositionSelection> m_select_position;
    bool m_adjust_gps {false};
    bool m_position_select_vertical {false};

    uint32_t m_next_redraw_time {0};
};
