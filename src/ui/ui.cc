#include "ui.hh"

#include "boat.hh"
#include "numbers.hh"
#include "painter.hh"
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
    , m_boat(DecodePng(boat_data))
    , m_numbers(DecodePng(numbers_data))
{
    m_static_map_buffer = std::make_unique<uint16_t[]>(hal::kDisplayWidth * hal::kDisplayHeight);
    m_static_map_image = std::make_unique<Image>(
        std::span<const uint16_t> {m_static_map_buffer.get(),
                                   hal::kDisplayWidth * hal::kDisplayHeight},
        hal::kDisplayWidth,
        hal::kDisplayHeight);

    input.AttachListener(this);
    m_gps_port->AwakeOn(GetSemaphore());
    m_route_listener->AwakeOn(GetSemaphore());

    m_position = {9210, 6000};
    m_map_position = PositionToMapCenter(m_position);
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


    m_route_line = std::make_unique<RouteLine>();

    m_speedometer_arc = std::make_unique<LvWrapper<lv_obj_t>>(lv_arc_create(lv_screen_active()));
    lv_obj_set_size(m_speedometer_arc->obj, lv_pct(100), lv_pct(100));
    lv_obj_remove_style(m_speedometer_arc->obj, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(m_speedometer_arc->obj, LV_OBJ_FLAG_CLICKABLE);
    //    lv_obj_set_style_arc_color(m_speedometer_arc->obj,
    //                               lv_color_t {.red = 50, .blue = 155, .green = 255},
    //                               LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(m_speedometer_arc->obj, 30, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(m_speedometer_arc->obj, false, LV_PART_INDICATOR);
    lv_arc_set_rotation(m_speedometer_arc->obj, 135);
    lv_arc_set_value(m_speedometer_arc->obj, 100);
    lv_obj_center(m_speedometer_arc->obj);


    m_speedometer_scale =
        std::make_unique<LvWrapper<lv_obj_t>>(lv_scale_create(lv_screen_active()));

    lv_obj_set_size(m_speedometer_scale->obj, lv_pct(100), lv_pct(100));
    lv_scale_set_mode(m_speedometer_scale->obj, LV_SCALE_MODE_ROUND_INNER);
    lv_obj_set_style_bg_opa(m_speedometer_scale->obj, LV_OPA_0, 0);
    lv_obj_set_style_bg_color(m_speedometer_scale->obj, lv_palette_lighten(LV_PALETTE_RED, 5), 0);
    lv_obj_set_style_radius(m_speedometer_scale->obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_clip_corner(m_speedometer_scale->obj, true, 0);
    lv_obj_align(m_speedometer_scale->obj, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_scale_set_label_show(m_speedometer_scale->obj, true);

    lv_scale_set_total_tick_count(m_speedometer_scale->obj, 41);
    lv_scale_set_major_tick_every(m_speedometer_scale->obj, 5);

    lv_obj_set_style_length(m_speedometer_scale->obj, 5, LV_PART_ITEMS);
    lv_obj_set_style_length(m_speedometer_scale->obj, 10, LV_PART_INDICATOR);
    lv_scale_set_range(m_speedometer_scale->obj, 0, 40);

    lv_scale_set_angle_range(m_speedometer_scale->obj, 270);
    lv_scale_set_rotation(m_speedometer_scale->obj, 135);

    m_boat_lv_img = std::make_unique<LvWrapper<lv_obj_t>>(lv_image_create(lv_screen_active()));
    lv_image_set_src(m_boat_lv_img->obj, &m_boat->lv_image_dsc);
    lv_obj_center(m_boat_lv_img->obj);
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
        // Update the boat position in global pixel coordinates
        m_position = position->pixel_position;
        m_map_position = PositionToMapCenter(m_position);

        m_speed = position->speed;

        lv_image_set_rotation(m_boat_lv_img->obj, position->heading * 10);
    }

    RunStateMachine();

    DrawMap();
    DrawRoute();
    DrawBoat();

    if (m_show_speedometer)
    {
        DrawSpeedometer();
    }

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
                auto img = lv_image_create(lv_screen_active());

                lv_obj_move_background(img);
                lv_image_set_src(img, &tile->GetImage().lv_image_dsc);
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
        //        memcpy(m_frame_buffer,
        //               m_static_map_buffer.get(),
        //               hal::kDisplayWidth * hal::kDisplayHeight * sizeof(uint16_t));
    }
}

void
UserInterface::DrawRoute()
{
    m_route_line->points.clear();
    lv_line_set_points(m_route_line->lv_line->obj, {}, 0);
    if (m_route.empty())
    {
        return;
    }

    auto index = 0;
    auto route_iterator = RouteIterator(m_route, m_land_mask_row_size);
    auto last_point = route_iterator.Next();

    m_route_line->points.push_back(
        {last_point->x - m_map_position.x, last_point->y - m_map_position.y});

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
            m_route_line->points.push_back(
                {cur_point->x - m_map_position.x, cur_point->y - m_map_position.y});
        }
        else
        {
            //            painter::DrawAlphaLine(m_frame_buffer,
            //                                   Point {last_point->x - m_map_position_zoomed_out.x,
            //                                          last_point->y - m_map_position_zoomed_out.y} /
            //                                       m_zoom_level,
            //                                   Point {cur_point->x - m_map_position_zoomed_out.x,
            //                                          cur_point->y - m_map_position_zoomed_out.y} /
            //                                       m_zoom_level,
            //                                   8,
            //                                   color,
            //                                   128);
        }

        last_point = cur_point;
        index++;
    }

    lv_line_set_points(
        m_route_line->lv_line->obj, m_route_line->points.data(), m_route_line->points.size());

    // TMP
    /*Create style*/
    static lv_style_t style_line;
    lv_style_init(&style_line);
    lv_style_set_line_width(&style_line, 8);
    lv_style_set_line_color(&style_line, lv_palette_main(LV_PALETTE_LIGHT_GREEN));
    lv_style_set_line_rounded(&style_line, true);

    lv_obj_add_style(m_route_line->lv_line->obj, &style_line, 0);
}

void
UserInterface::DrawBoat()
{
    int32_t x;
    int32_t y;

    if (m_state == State::kMap)
    {
        x = m_position.x - m_map_position.x;
        y = m_position.y - m_map_position.y;
    }
    else
    {
        x = (m_position.x - m_map_position_zoomed_out.x) / m_zoom_level;
        y = (m_position.y - m_map_position_zoomed_out.y) / m_zoom_level;
    }

    lv_obj_align(m_boat_lv_img->obj,
                 LV_ALIGN_TOP_LEFT,
                 x - m_boat->lv_image_dsc.header.w / 2,
                 y - m_boat->lv_image_dsc.header.h / 2);
}

void
UserInterface::DrawSpeedometer()
{
    constexpr auto max_speed = 40.0f;
    constexpr auto max_angle = 270.0f;
    auto speed = std::min(m_speed, max_speed);

    auto angle = speed * max_angle / max_speed;

    lv_arc_set_bg_angles(m_speedometer_arc->obj, 0, angle);
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
