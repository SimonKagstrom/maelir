#include "ui.hh"

#include "map_screen.hh"
#include "menu_screen.hh"
#include "route_iterator.hh"
#include "route_utils.hh"
#include "time.hh"
#include "updating_screen.hh"
#include "version.hh"

#include <cmath>
#include <numbers>


UserInterface::UserInterface(ApplicationState& application_state,
                             const MapMetadata& metadata,
                             TileProducer& tile_producer,
                             hal::IDisplay& display,
                             hal::IInput& input,
                             RouteService& route_service,
                             OtaUpdater& ota_updater,
                             std::unique_ptr<IGpsPort> gps_port,
                             std::unique_ptr<IRouteListener> route_listener)
    : m_tile_rows(metadata.tile_rows)
    , m_tile_row_size(metadata.tile_row_size)
    , m_land_mask_rows(metadata.land_mask_rows)
    , m_land_mask_row_size(metadata.land_mask_row_size)
    , m_application_state(application_state)
    , m_tile_producer(tile_producer)
    , m_display(display)
    , m_input(input)
    , m_route_service(route_service)
    , m_ota_updater(ota_updater)
    , m_gps_port(std::move(gps_port))
    , m_route_listener(std::move(route_listener))
    , m_gps_position_timer(StartTimer(0ms)) // Timeout directly, but always valid
{
    input.AttachListener(this);
    m_gps_port->AwakeOn(GetSemaphore());
    m_route_listener->AwakeOn(GetSemaphore());
}

void
UserInterface::StaticLvglFlushCallback(lv_display_t* display,
                                       const lv_area_t* area [[maybe_unused]],
                                       uint8_t* px_map)
{
    if (lv_display_flush_is_last(display))
    {
        auto p = static_cast<UserInterface*>(lv_display_get_user_data(display));

        p->m_display.Flip();
        lv_display_flush_ready(display);
    }
}


void
UserInterface::OnStartup()
{
    assert(m_lvgl_display == nullptr);

    m_position = LandIndexToPoint(m_application_state.CheckoutReadonly()->home_position,
                                  m_land_mask_row_size);

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
    lv_display_set_flush_cb(m_lvgl_display, StaticLvglFlushCallback);


    m_lvgl_input_dev = lv_indev_create();
    lv_indev_set_mode(m_lvgl_input_dev, LV_INDEV_MODE_EVENT);
    lv_indev_set_type(m_lvgl_input_dev, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_user_data(m_lvgl_input_dev, this);
    lv_indev_set_read_cb(m_lvgl_input_dev, StaticLvglEncoderRead);

    if (m_ota_updater.ApplicationHasBeenUpdated())
    {
        auto mbox = lv_msgbox_create(NULL);

        lv_msgbox_add_title(mbox, "Software updated");
        lv_msgbox_add_text(
            mbox,
            std::format(
                "The software has been updated to {}. Will be marked valid after 10 seconds",
                kSoftwareVersion)
                .c_str());

        m_updated_timer = StartTimer(5s, [mbox]() {
            lv_msgbox_close(mbox);
            return std::nullopt;
        });
    }

    m_map_screen = std::make_unique<MapScreen>(*this);
    m_map_screen->Activate();
}

void
UserInterface::StaticLvglEncoderRead(lv_indev_t* indev, lv_indev_data_t* data)
{
    auto p = static_cast<UserInterface*>(lv_indev_get_user_data(indev));

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
        default:
            break;
        }

        auto on_map_screen = m_menu_screen == nullptr && m_updating_screen == nullptr;
        m_current_screen->OnInput(event);

        if (!on_map_screen)
        {
            // OnInput on the main screen can trigger a menu entry, so avoid an immediate exit
            lv_indev_read(m_lvgl_input_dev);
        }
    }


    while (auto route = m_route_listener->Poll())
    {
        m_passed_route_index = -1;
        if (route->type == IRouteListener::EventType::kReady)
        {
            m_route.clear();
            std::ranges::copy(route->route, std::back_inserter(m_route));
            m_calculating_route = false;
        }
        else
        {
            // For now always
            m_calculating_route = route->type == IRouteListener::EventType::kCalculating;

            m_route.clear();
        }
    }

    if (auto position = m_gps_port->Poll())
    {
        m_position = position->pixel_position;
        m_speed = position->speed;

        m_gps_position_timer = StartTimer(5s);

        m_map_screen->OnPosition(*position);
    }
    m_gps_position_valid = !m_gps_position_timer->IsExpired();

    m_current_screen->Update();

    if (auto time_before = os::GetTimeStampRaw(); m_next_redraw_time > time_before)
    {
        // Wait for the next redraw
        return milliseconds(m_next_redraw_time - time_before);
    }
    auto delay = lv_timer_handler();
    m_next_redraw_time = os::GetTimeStampRaw() + delay;

    return milliseconds(delay);
}

void
UserInterface::EnterMenu()
{
    m_gps_port->DisableWakeup();
    m_menu_screen = std::make_unique<MenuScreen>(*this, [this]() {
        if (m_updating_screen == nullptr)
        {
            // Ugly
            m_map_screen->Activate();
        }
        m_deferred_deletion = Defer([this]() {
            m_menu_screen = nullptr;

            return std::nullopt;
        });
        m_gps_port->AwakeOn(GetSemaphore());
    });

    m_menu_screen->Activate();
}

void
UserInterface::EnterOtaUpdatingScreen()
{
    m_gps_port->DisableWakeup();

    m_updating_screen = std::make_unique<UpdatingScreen>(*this);
    m_updating_screen->Activate();
}

void
UserInterface::OnInput(const hal::IInput::Event& event)
{
    m_input_queue.push(event);
    Awake();
}

void
UserInterface::SelectPosition(PositionSelection selection)
{
    m_select_position = selection;
    m_position_select_vertical = false;
}


ScreenBase::ScreenBase(UserInterface& parent)
    : m_parent(parent)
    , m_screen(lv_obj_create(nullptr))
{
}


ScreenBase::~ScreenBase()
{
    lv_obj_delete(m_screen);
}

void
ScreenBase::Activate()
{
    lv_screen_load(m_screen);
    m_parent.m_current_screen = this;
}
