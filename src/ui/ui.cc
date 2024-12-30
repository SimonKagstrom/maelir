#include "ui.hh"

#include "boat.hh"
#include "numbers.hh"
#include "painter.hh"
#include "route_iterator.hh"
#include "route_utils.hh"
#include "time.hh"

#include <cmath>
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
    , m_boat(DecodePng(boat_data))
    , m_numbers(DecodePng(numbers_data))
    , m_boat_rotation(painter::AllocateRotationBuffer(*m_boat))
    , m_rotated_boat(*m_boat)
{
    m_static_map_buffer = std::make_unique<uint16_t[]>(hal::kDisplayWidth * hal::kDisplayHeight);
    m_static_map_image = std::make_unique<Image>(
        std::span<const uint16_t> {m_static_map_buffer.get(),
                                   hal::kDisplayWidth * hal::kDisplayHeight},
        hal::kDisplayWidth,
        hal::kDisplayHeight);
    m_rotated_boat = painter::Rotate(*m_boat, m_boat_rotation, 0);

    input.AttachListener(this);
    m_gps_port->AwakeOn(GetSemaphore());
    m_route_listener->AwakeOn(GetSemaphore());

    m_position = {9210, 6000};
    m_map_position = PositionToMapCenter(m_position);
}

void
UserInterface::LvFlushCallback(const lv_area_t* area, uint8_t* px_map)
{
    memset(px_map, 0, area->x2 - area->x1 + 1);
    lv_display_flush_ready(m_lvgl_display);
}

void
UserInterface::LvFlushCallbackStatic(lv_display_t* display, const lv_area_t* area, uint8_t* px_map)
{
    auto pThis = static_cast<UserInterface*>(lv_display_get_user_data(display));
    pThis->LvFlushCallback(area, px_map);
}


std::optional<milliseconds>
UserInterface::OnActivation()
{
    if (!m_lvgl_display)
    {
        lv_init();
        lv_tick_set_cb(ms_to_ticks);

        m_lvgl_display = lv_display_create(hal::kDisplayWidth, hal::kDisplayHeight);
        lv_display_set_buffers(m_lvgl_display,
                               m_display.GetFrameBuffer(),
                               nullptr,
                               sizeof(uint16_t) * hal::kDisplayWidth * hal::kDisplayHeight,
                               lv_display_render_mode_t::LV_DISPLAY_RENDER_MODE_FULL);
        lv_display_set_user_data(m_lvgl_display, this);
    }


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
        // Update the boat position in global pixel coordinates
        m_position = position->pixel_position;
        m_map_position = PositionToMapCenter(m_position);

        m_speed = position->speed;

        m_rotated_boat = painter::Rotate(*m_boat, m_boat_rotation, position->heading);
    }

    RunStateMachine();

    // Can potentially have to wait
    m_frame_buffer = m_display.GetFrameBuffer();

    DrawMap();
    DrawRoute();
    DrawBoat();

    if (m_show_speedometer)
    {
        DrawSpeedometer();
    }


    printf("da tajma\n");
    lv_refr_now(nullptr);
    auto delay = lv_timer_handler();

    printf("Flipping\n");
    m_display.Flip();
    // Now invalid
    m_frame_buffer = nullptr;

    printf("Soon restart\n");
    return milliseconds(delay);
}


void
UserInterface::OnInput(const hal::IInput::Event& event)
{
    m_input_queue.push(event);
    Awake();
}

void
UserInterface::RequestMapTiles(const Point& position)
{
    m_tiles.clear();

    auto x_remainder = position.x % kTileSize;
    auto y_remainder = position.y % kTileSize;
    auto num_tiles_x = (hal::kDisplayWidth + kTileSize - 1) / kTileSize + !!x_remainder;
    auto num_tiles_y = (hal::kDisplayHeight + kTileSize - 1) / kTileSize + !!y_remainder;

    auto start_x = position.x - x_remainder;
    auto start_y = position.y - y_remainder;

    // Blit all needed tiles
    for (auto y = 0; y < num_tiles_y; y++)
    {
        for (auto x = 0; x < num_tiles_x; x++)
        {
            auto tile =
                m_tile_producer.LockTile({start_x + x * kTileSize, start_y + y * kTileSize});
            if (tile)
            {
                auto img = lv_img_create(lv_screen_active());

                lv_img_set_src(img, &tile->GetImage().lv_image_dsc);
                lv_obj_align(img,
                             LV_ALIGN_TOP_LEFT,
                             x * kTileSize - x_remainder,
                             y * kTileSize - y_remainder);

                m_tiles.emplace_back(std::move(tile), img);
            }
        }
    }
}

void
UserInterface::PrepareInitialZoomedOutMap()
{
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
}

void
UserInterface::DrawZoomedTile(const Point& position)
{
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
            RequestMapTiles(m_map_position);

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
            else if (m_position.x < m_map_position_zoomed_out.x + 30 ||
                     m_position.y < m_map_position_zoomed_out.y + 30 ||
                     m_position.x >
                         m_map_position_zoomed_out.x + hal::kDisplayWidth * m_zoom_level - 30 ||
                     m_position.y >
                         m_map_position_zoomed_out.y + hal::kDisplayHeight * m_zoom_level - 30)
            {
                // Boat outside the visible area
                m_state = State::kInitialOverviewMap;
            }
            break;

        case State::kValueCount:
            break;
        }

        before = m_state;
    } while (m_state != before);
}

void
UserInterface::DrawMap()
{
    if (m_state == State::kMap)
    {
        for (const auto& [tile, position] : m_tiles)
        {
            //            painter::Blit(m_frame_buffer, tile->GetImage(), {position.x, position.y});
        }
    }
    else
    {
        memcpy(m_frame_buffer,
               m_static_map_buffer.get(),
               hal::kDisplayWidth * hal::kDisplayHeight * sizeof(uint16_t));
    }
}

void
UserInterface::DrawRoute()
{
    if (m_route.empty())
    {
        return;
    }

    auto index = 0;
    auto route_iterator = RouteIterator(m_route, m_land_mask_row_size);

    auto last_point = route_iterator.Next();
    while (auto cur_point = route_iterator.Next())
    {
        constexpr auto kThreshold = 3 * kPathFinderTileSize;

        // Mark the route as passed if the boat is close enough
        if (std::abs(m_position.x - cur_point->x) < kThreshold &&
            std::abs(m_position.y - cur_point->y) < kThreshold)
        {
            if (!m_passed_route_index || *m_passed_route_index < index)
            {
                m_passed_route_index = index;
            }
        }

        auto color = m_passed_route_index && index < *m_passed_route_index
                         ? 0x7BEF
                         : 0x07E0; // Green in RGB565

        if (m_state == State::kMap)
        {
            painter::DrawAlphaLine(
                m_frame_buffer,
                {last_point->x - m_map_position.x, last_point->y - m_map_position.y},
                {cur_point->x - m_map_position.x, cur_point->y - m_map_position.y},
                8,
                color,
                128);
        }
        else
        {
            painter::DrawAlphaLine(m_frame_buffer,
                                   Point {last_point->x - m_map_position_zoomed_out.x,
                                          last_point->y - m_map_position_zoomed_out.y} /
                                       m_zoom_level,
                                   Point {cur_point->x - m_map_position_zoomed_out.x,
                                          cur_point->y - m_map_position_zoomed_out.y} /
                                       m_zoom_level,
                                   8,
                                   color,
                                   128);
        }

        last_point = cur_point;
        index++;
    }
}

void
UserInterface::DrawBoat()
{
    int32_t x;
    int32_t y;

    if (m_state == State::kMap)
    {
        x = m_position.x - m_map_position.x - m_rotated_boat.width / 2;
        y = m_position.y - m_map_position.y - m_rotated_boat.height / 2;
    }
    else
    {
        x = (m_position.x - m_map_position_zoomed_out.x) / m_zoom_level - m_rotated_boat.width / 2;
        y = (m_position.y - m_map_position_zoomed_out.y) / m_zoom_level - m_rotated_boat.height / 2;
    }

    painter::MaskBlit(m_frame_buffer, m_rotated_boat, {x, y});
}

void
UserInterface::DrawSpeedometer()
{
    char speed_str[3] {};

    speed_str[0] = '0' + static_cast<int>(m_speed) / 10 % 10;
    speed_str[1] = '0' + static_cast<int>(m_speed) % 10;

    const auto x = hal::kDisplayWidth / 2 - m_numbers->width / 10;
    const auto y = 20;

    painter::DrawNumbers(m_frame_buffer, *m_numbers, {x, y}, speed_str);
}

Point
UserInterface::PositionToMapCenter(const auto& pixel_position) const
{
    auto x = pixel_position.x;
    auto y = pixel_position.y;

    x = std::clamp(static_cast<int>(x - hal::kDisplayWidth / 2),
                   0,
                   static_cast<int>(m_tile_row_size) * kTileSize - hal::kDisplayWidth);
    y = std::clamp(static_cast<int>(y - hal::kDisplayHeight / 2),
                   0,
                   static_cast<int>(m_tile_row_size) * kTileSize - hal::kDisplayHeight);

    return {x, y};
}
