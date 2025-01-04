#include "ui.hh"

#include "map_screen.hh"
#include "route_iterator.hh"
#include "route_utils.hh"
#include "time.hh"

#include <cmath>
#include <fmt/format.h>
#include <numbers>


namespace
{

uint32_t
ms_to_ticks(void)
{
    auto out = os::GetTimeStamp();

    return out.count();
}

} // namespace


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
    lv_tick_set_cb(ms_to_ticks);

    m_lvgl_display = lv_display_create(hal::kDisplayWidth, hal::kDisplayHeight);
    auto f1 = m_display.GetFrameBuffer(hal::IDisplay::Owner::kSoftware);
    auto f2 = m_display.GetFrameBuffer(hal::IDisplay::Owner::kHardware);

    lv_display_set_buffers(m_lvgl_display,
                           f1,
                           f2,
                           sizeof(uint16_t) * hal::kDisplayWidth * hal::kDisplayHeight,
                           lv_display_render_mode_t::LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_user_data(m_lvgl_display, this);

    m_map_screen = std::make_unique<MapScreen>(*this);
    m_map_screen->Activate();
}

std::optional<milliseconds>
UserInterface::OnActivation()
{
    // Handle input
    hal::IInput::Event event;
    while (m_input_queue.pop(event))
    {
        auto mode = std::to_underlying(m_mode);

        switch (event.type)
        {
        case hal::IInput::EventType::kButtonDown:
            m_button_timer = nullptr;
            m_button_timer = StartTimer(5s, [this]() {
                auto state = m_application_state.Checkout();

                state->demo_mode = !state->demo_mode;

                return std::nullopt;
            });
            break;
        case hal::IInput::EventType::kButtonUp:
            if (m_button_timer && (m_button_timer->TimeLeft() > 4500ms))
            {
                m_show_speedometer = !m_show_speedometer;
            }
            m_button_timer = nullptr;
            break;
        case hal::IInput::EventType::kLeft:
            mode--;
            break;
        case hal::IInput::EventType::kRight:
            mode++;
            break;
        default:
            break;
        }
        printf("Event: %d. State now %d\n", (int)event.type, (int)mode);
        m_mode = static_cast<Mode>(mode % std::to_underlying(Mode::kValueCount));
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
UserInterface::OnInput(const hal::IInput::Event& event)
{
    m_input_queue.push(event);
    Awake();
}
