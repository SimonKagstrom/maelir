#include "ui.hh"

#include "map_screen.hh"
#include "menu_screen.hh"
#include "route_iterator.hh"
#include "route_utils.hh"
#include "time.hh"

#include <cmath>
#include <fmt/format.h>
#include <numbers>


UserInterface::UserInterface(ApplicationState& application_state,
                             const MapMetadata& metadata,
                             TileProducer& tile_producer,
                             hal::IDisplay& display,
                             hal::IInput& input,
                             std::unique_ptr<IGpsPort> gps_port,
                             std::unique_ptr<IRouteListener> route_listener)
    : m_tile_rows(metadata.tile_rows)
    , m_tile_row_size(metadata.tile_row_size)
    , m_land_mask_rows(metadata.land_mask_rows)
    , m_land_mask_row_size(metadata.land_mask_row_size)
    , m_application_state(application_state)
    , m_tile_producer(tile_producer)
    , m_display(display)
    , m_gps_port(std::move(gps_port))
    , m_route_listener(std::move(route_listener))
{
    input.AttachListener(this);
    m_gps_port->AwakeOn(GetSemaphore());
    m_route_listener->AwakeOn(GetSemaphore());

    m_position = {9210, 6000};
}


void
UserInterface::OnStartup()
{
    assert(m_lvgl_display == nullptr);

    lv_init();
    lv_tick_set_cb(os::GetTimeStampRaw);

    m_lvgl_display = lv_display_create(hal::kDisplayWidth, hal::kDisplayHeight);
    auto f1 = m_display.GetFrameBuffer(hal::IDisplay::Owner::kSoftware);
    auto f2 = m_display.GetFrameBuffer(hal::IDisplay::Owner::kHardware);

    lv_display_set_buffers(m_lvgl_display,
                           f1,
                           f2,
                           sizeof(uint16_t) * hal::kDisplayWidth * hal::kDisplayHeight,
                           lv_display_render_mode_t::LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_user_data(m_lvgl_display, this);


    m_lvgl_input_dev = lv_indev_create();
    lv_indev_set_mode(m_lvgl_input_dev, LV_INDEV_MODE_EVENT);
    lv_indev_set_type(m_lvgl_input_dev, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_user_data(m_lvgl_input_dev, this);
    lv_indev_set_read_cb(m_lvgl_input_dev, StaticLvglEncoderRead);

    m_map_screen = std::make_unique<MapScreen>(*this);
    m_map_screen->Activate();
}

void
UserInterface::StaticLvglEncoderRead(lv_indev_t* indev, lv_indev_data_t* data)
{
    auto p = reinterpret_cast<UserInterface*>(lv_indev_get_user_data(indev));

    data->state = p->m_button_state;
    data->enc_diff = p->m_enc_diff;
}

std::optional<milliseconds>
UserInterface::OnActivation()
{
    // Handle input
    hal::IInput::Event event;
    while (m_input_queue.pop(event))
    {
        m_enc_diff = 0;

        switch (event.type)
        {
        case hal::IInput::EventType::kButtonDown:
            m_button_state = LV_INDEV_STATE_PRESSED;
            break;
        case hal::IInput::EventType::kButtonUp:
            m_button_state = LV_INDEV_STATE_RELEASED;
            break;
        case hal::IInput::EventType::kLeft:
            m_enc_diff = -1;
            break;
        case hal::IInput::EventType::kRight:
            m_enc_diff = 1;
            break;
        case hal::IInput::EventType::kValueCount:
            break;
        }

        lv_indev_read(m_lvgl_input_dev);

        // Ugly
        if (m_menu_screen == nullptr)
        {
            m_map_screen->OnInput(event);
        }
    }


    while (auto route = m_route_listener->Poll())
    {
        if (route->type == IRouteListener::EventType::kReady)
        {
            m_route.clear();
            std::ranges::copy(route->route, std::back_inserter(m_route));
        }
        else
        {
            m_route.clear();
            m_passed_route_index = std::nullopt;
        }
    }

    if (auto position = m_gps_port->Poll())
    {
        m_position = position->pixel_position;
        m_speed = position->speed;

        m_map_screen->OnPosition(*position);
    }

    m_map_screen->Update();

    auto delay = lv_timer_handler();

    // Display the frame buffer and let LVGL know that it's done
    m_display.Flip();
    lv_display_flush_ready(m_lvgl_display);

    return milliseconds(delay);
}

void
UserInterface::EnterMenu()
{
    m_menu_screen = std::make_unique<MenuScreen>(*this, [this]() {
        m_map_screen->Activate();
        m_menu_screen = nullptr;
    });

    m_menu_screen->Activate();
}

void
UserInterface::OnInput(const hal::IInput::Event& event)
{
    m_input_queue.push(event);
    Awake();
}
