#include "ui.hh"

#include "map_screen.hh"
#include "numbers.hh"
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
    m_static_map_buffer =
        std::make_unique<uint8_t[]>(hal::kDisplayWidth * hal::kDisplayHeight * sizeof(uint16_t));
    m_static_map_image = std::make_unique<Image>(
        std::span<const uint8_t> {m_static_map_buffer.get(),
                                  hal::kDisplayWidth * hal::kDisplayHeight * sizeof(uint16_t)},
        hal::kDisplayWidth,
        hal::kDisplayHeight);

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

    RunStateMachine();
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

void
UserInterface::PrepareInitialZoomedOutMap()
{
#if 0
    // Free old tiles and fill with black
    m_zoomed_out_map_tiles.clear();
    memset(
        m_static_map_buffer.get(), 0, hal::kDisplayWidth * hal::kDisplayHeight * sizeof(uint16_t));

    // Align with the nearest tile
    auto aligned =
        Point {m_position.x - m_position.x % kTileSize, m_position.y - m_position.y % kTileSize};

    const auto offset_x =
        std::max(static_cast<int32_t>(0), aligned.x - (m_zoom_level * hal::kDisplayWidth) / 2);
    const auto offset_y =
        std::max(static_cast<int32_t>(0), aligned.y - (m_zoom_level * hal::kDisplayHeight) / 2);
    m_map_position_zoomed_out = Point {offset_x, offset_y};

    auto num_tiles_x = hal::kDisplayWidth / (kTileSize / m_zoom_level);
    auto num_tiles_y = hal::kDisplayHeight / (kTileSize / m_zoom_level);

    constexpr auto kVisibleTiles = hal::kDisplayWidth / kTileSize;
    etl::vector<Point, kVisibleTiles * kVisibleTiles> cached_tiles;

    for (auto y = 0; y < num_tiles_y; y++)
    {
        for (auto x = 0; x < num_tiles_x; x++)
        {
            auto at = Point {offset_x + x * kTileSize, offset_y + y * kTileSize};

            if (m_tile_producer.IsCached(at) && cached_tiles.full() == false)
            {
                cached_tiles.push_back(at);
            }
            else
            {
                m_zoomed_out_map_tiles.push_back(at);
            }
        }
    }

    for (const auto& position : cached_tiles)
    {
        DrawZoomedTile(position);
    }

    m_tiles.clear();
    Awake();
#endif
}

void
UserInterface::DrawZoomedTile(const Point& position)
{
#if 0
    auto tile = m_tile_producer.LockTile(position);
    if (tile)
    {
        auto dst = Point {position.x - m_map_position_zoomed_out.x,
                          position.y - m_map_position_zoomed_out.y};
                painter::ZoomedBlit(m_static_map_buffer.get(),
                                    tile->GetImage(),
                                    m_zoom_level,
                                    {dst.x / m_zoom_level, dst.y / m_zoom_level});
    }
#endif
}

void
UserInterface::FillZoomedOutMap()
{
    const auto num_tiles_x = hal::kDisplayWidth / (kTileSize / m_zoom_level);
    for (auto i = 0; i < num_tiles_x; i++)
    {
        if (m_zoomed_out_map_tiles.empty())
        {
            break;
        }

        auto position = m_zoomed_out_map_tiles.back();
        DrawZoomedTile(position);
        m_zoomed_out_map_tiles.pop_back();
    }

    Awake();
}


void
UserInterface::RunStateMachine()
{
    auto before = m_state;

    auto zoom_mismatch = [this]() {
        return m_zoom_level == 2 && m_mode == Mode::kZoom4 ||
               m_zoom_level == 4 && m_mode == Mode::kZoom2;
    };

    do
    {
        switch (m_state)
        {
        case State::kMap:
            m_zoom_level = 1;
            //            RequestMapTiles(m_map_position);

            if (m_mode != Mode::kMap)
            {
                // Always go through this
                m_state = State::kInitialOverviewMap;
            }
            break;

        case State::kInitialOverviewMap:
            m_zoom_level = m_mode == Mode::kZoom2 ? 2 : 4;
            PrepareInitialZoomedOutMap();

            m_state = State::kFillOverviewMapTiles;

            break;
        case State::kFillOverviewMapTiles:
            FillZoomedOutMap();

            if (m_zoomed_out_map_tiles.empty())
            {
                m_state = State::kOverviewMap;
            }
            else if (m_mode == Mode::kMap)
            {
                m_state = State::kMap;
            }
            else if (zoom_mismatch())
            {
                m_state = State::kInitialOverviewMap;
            }
            break;

        case State::kOverviewMap:
            if (m_mode == Mode::kMap)
            {
                m_state = State::kMap;
            }
            else if (zoom_mismatch())
            {
                m_state = State::kInitialOverviewMap;
            }
            //            else if (m_position.x < m_map_position_zoomed_out.x + 30 ||
            //                     m_position.y < m_map_position_zoomed_out.y + 30 ||
            //                     m_position.x >
            //                         m_map_position_zoomed_out.x + hal::kDisplayWidth * m_zoom_level - 30 ||
            //                     m_position.y >
            //                         m_map_position_zoomed_out.y + hal::kDisplayHeight * m_zoom_level - 30)
            //            {
            //                // Boat outside the visible area
            //                m_state = State::kInitialOverviewMap;
            //            }
            break;

        case State::kValueCount:
            break;
        }

        before = m_state;
    } while (m_state != before);
}

//void
//UserInterface::DrawMap()
//{
//    if (m_state == State::kMap)
//    {
//    }
//    else
//    {
//                memcpy(m_frame_buffer,
//                       m_static_map_buffer.get(),
//                       hal::kDisplayWidth * hal::kDisplayHeight * sizeof(uint16_t));
//    }
//}
//
