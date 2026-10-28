#include "map_screen.hh"

#include "boat.hh"
#include "cohen_sutherland.hh"
#include "crosshair.hh"
#include "painter.hh"
#include "route_utils.hh"

#include <cstring>

constexpr auto kMaxKnots = 30;
constexpr auto kSpeedometerMaxAngle = 202;

UserInterface::MapScreen::MapScreen(UserInterface& parent)
    : ScreenBase(parent)
    , m_boat_data(DecodePngMask(boat_data, 0))
    , m_crosshair_data(DecodePngMask(crosshair_data, 0))
    , m_select_timer(m_parent.StartTimer(0ms))
{
    m_static_map_buffer = std::make_unique<uint16_t[]>(hal::kDisplayWidth * hal::kDisplayHeight);

    m_map_position = PositionToMapCenter(m_parent.m_position);

    // The map is painted below everything else, so the screen background is never seen
    lv_obj_set_style_bg_opa(m_screen, LV_OPA_TRANSP, 0);
    lv_obj_set_scrollbar_mode(m_screen, LV_SCROLLBAR_MODE_OFF);

    /*
     * The map and route are drawn directly into the LVGL render buffer, which avoids an
     * intermediate full-screen buffer and the copy from it (PSRAM bandwidth is the limit).
     */
    lv_obj_add_event_cb(
        m_screen,
        [](lv_event_t* e) {
            auto* self = static_cast<MapScreen*>(lv_event_get_user_data(e));
            auto* layer = lv_event_get_layer(e);

            self->DrawBackground(static_cast<uint16_t*>(static_cast<void*>(layer->draw_buf->data)));
        },
        LV_EVENT_DRAW_MAIN,
        this);

    m_speedometer_arc = lv_arc_create(m_screen);
    lv_obj_set_size(m_speedometer_arc, lv_pct(100), lv_pct(100));
    lv_obj_remove_style(m_speedometer_arc, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(m_speedometer_arc, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_set_style_arc_width(m_speedometer_arc, 22, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(m_speedometer_arc, false, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(
        m_speedometer_arc, lv_palette_main(LV_PALETTE_LIGHT_GREEN), LV_PART_INDICATOR);
    lv_arc_set_rotation(m_speedometer_arc, 135);
    lv_arc_set_value(m_speedometer_arc, 100);
    lv_obj_center(m_speedometer_arc);


    m_speedometer_scale = lv_scale_create(m_screen);

    lv_obj_set_size(m_speedometer_scale, lv_pct(100), lv_pct(100));
    lv_scale_set_mode(m_speedometer_scale, LV_SCALE_MODE_ROUND_INNER);
    lv_obj_set_style_bg_opa(m_speedometer_scale, LV_OPA_0, 0);
    lv_obj_set_style_bg_color(m_speedometer_scale, lv_palette_lighten(LV_PALETTE_RED, 5), 0);
    lv_obj_set_style_radius(m_speedometer_scale, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_clip_corner(m_speedometer_scale, true, 0);
    lv_obj_align(m_speedometer_scale, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_scale_set_label_show(m_speedometer_scale, true);
    lv_obj_set_style_text_font(m_speedometer_scale, &lv_font_montserrat_30, LV_PART_MAIN);

    lv_scale_set_total_tick_count(m_speedometer_scale, kMaxKnots + 1);
    lv_scale_set_major_tick_every(m_speedometer_scale, 5);

    lv_obj_set_style_length(m_speedometer_scale, 10, LV_PART_ITEMS);
    lv_obj_set_style_length(m_speedometer_scale, 20, LV_PART_INDICATOR);
    lv_obj_set_style_width(m_speedometer_scale, 2, LV_PART_ITEMS);
    lv_obj_set_style_width(m_speedometer_scale, 2, LV_PART_INDICATOR);
    lv_scale_set_range(m_speedometer_scale, 0, kMaxKnots);

    lv_scale_set_angle_range(m_speedometer_scale, kSpeedometerMaxAngle);
    lv_scale_set_rotation(m_speedometer_scale, 135);

    m_boat = lv_image_create(m_screen);
    lv_image_set_src(m_boat, &m_boat_data->lv_image_dsc);
    lv_obj_center(m_boat);

    m_crosshair = lv_image_create(m_screen);
    lv_image_set_src(m_crosshair, &m_crosshair_data->lv_image_dsc);
    lv_obj_center(m_crosshair);
    lv_obj_add_flag(m_crosshair, LV_OBJ_FLAG_HIDDEN);

    static lv_style_t style_white_text;
    lv_style_init(&style_white_text);
    lv_style_set_text_color(&style_white_text, lv_palette_main(LV_PALETTE_LIGHT_BLUE));

    static lv_style_t style_text_shadow;
    lv_style_init(&style_text_shadow);
    lv_style_set_text_color(&style_text_shadow, lv_color_black());

    m_indicators_shadow = lv_label_create(m_screen);
    m_indicators = lv_label_create(m_screen);

    lv_obj_add_style(m_indicators, &style_white_text, 0);
    lv_obj_add_style(m_indicators_shadow, &style_text_shadow, 0);
    lv_obj_set_style_text_font(m_indicators, &lv_font_montserrat_30, LV_PART_MAIN);
    lv_obj_set_style_text_font(m_indicators_shadow, &lv_font_montserrat_30, LV_PART_MAIN);

    // The trip computer (shown on the zoomed out maps)
    m_trip_computer_box = lv_obj_create(m_screen);
    lv_obj_align(m_trip_computer_box, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_set_size(m_trip_computer_box, 125, 125);
    lv_obj_set_style_bg_color(m_trip_computer_box, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_bg_opa(m_trip_computer_box, LV_OPA_70, 0);
    lv_obj_set_style_radius(m_trip_computer_box, 10, 0);

    m_trip_computer_label = lv_label_create(m_trip_computer_box);
    lv_obj_set_style_text_font(m_trip_computer_label, &lv_font_montserrat_22, LV_PART_MAIN);

    lv_obj_align(m_indicators, LV_ALIGN_BOTTOM_MID, 0, -1);
}

void
UserInterface::MapScreen::OnPosition(const GpsData& position)
{
    // Update the boat position in global pixel coordinates
    m_map_position = PositionToMapCenter(m_parent.m_position);

    lv_image_set_rotation(m_boat, position.heading * 10);
}

void
UserInterface::MapScreen::Update()
{
    char buf[64];

    auto& state = m_parent.m_application_state;
    auto conf = state.Get<AS::configuration>();
    auto show_speedometer = conf->show_speedometer;
    auto show_trip_computer = m_zoom_level > 1 && m_state != State::kSelectDestination;

    if (m_state == State::kSelectDestination)
    {
        m_show_route = false;
        lv_obj_remove_flag(m_crosshair, LV_OBJ_FLAG_HIDDEN);

        if (m_parent.m_position_select_vertical)
        {
            snprintf(buf, sizeof(buf), "%s\n%s", LV_SYMBOL_UP, LV_SYMBOL_DOWN);
        }
        else
        {
            snprintf(buf, sizeof(buf), "%s%s", LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT);
        }

        show_speedometer = false;
    }
    else if (m_state == State::kAdjustGps)
    {
        m_show_route = false;
        lv_obj_remove_flag(m_crosshair, LV_OBJ_FLAG_HIDDEN);

        auto first_sym = LV_SYMBOL_LEFT;
        auto second_sym = LV_SYMBOL_RIGHT;

        if (m_parent.m_position_select_vertical)
        {
            first_sym = LV_SYMBOL_UP;
            second_sym = LV_SYMBOL_DOWN;
        }

        snprintf(buf,
                 sizeof(buf),
                 "x %d, y %d\n%s\n%s",
                 conf->longitude_adjustment,
                 conf->latitude_adjustment,
                 first_sym,
                 second_sym);

        show_speedometer = false;
    }
    else
    {
        snprintf(buf,
                 sizeof(buf),
                 "%s%s%s",
                 state.Get<AS::gps_status>() == GpsStatus::kPositionValid ? LV_SYMBOL_GPS
                                                                          : LV_SYMBOL_CLOSE,
                 m_parent.m_calculating_route ? " " : "",
                 m_parent.m_calculating_route ? LV_SYMBOL_LOOP : "");
    }
    lv_label_set_text(m_indicators, buf);
    lv_label_set_text(m_indicators_shadow, buf);
    lv_obj_align_to(m_indicators_shadow, m_indicators, LV_ALIGN_TOP_LEFT, 2, 2);
    lv_obj_remove_flag(m_indicators_shadow, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(m_indicators, LV_OBJ_FLAG_HIDDEN);

    if (show_trip_computer)
    {
        lv_obj_remove_flag(m_trip_computer_box, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(m_trip_computer_label, LV_OBJ_FLAG_HIDDEN);

        auto meters_left =
            state.Get<AS::route_total_meters>() - state.Get<AS::route_passed_meters>();
        auto average_knots = state.Get<AS::five_minute_average_speed>();
        uint32_t time_left = average_knots == 0 ? 0u : meters_left / ((average_knots * 1852) / 60);
        auto hours_left = time_left / 60;
        auto minutes_left = time_left % 60;
        auto distance_format = "m";

        if (meters_left >= 5000)
        {
            meters_left /= 1000;
            distance_format = "km";
        }

        if (time_left >= 60)
        {
            time_left /= 60;
        }
        snprintf(buf,
                 sizeof(buf),
                 "%d %s\n%d:%02d\n%d / %d kn",
                 meters_left,
                 distance_format,
                 hours_left,
                 minutes_left,
                 state.Get<AS::minute_average_speed>(),
                 state.Get<AS::five_minute_average_speed>());
        lv_label_set_text(m_trip_computer_label, buf);
    }
    else
    {
        lv_obj_add_flag(m_trip_computer_box, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(m_trip_computer_label, LV_OBJ_FLAG_HIDDEN);
    }

    if (show_speedometer)
    {
        lv_obj_remove_flag(m_speedometer_scale, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(m_speedometer_arc, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_add_flag(m_speedometer_scale, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(m_speedometer_arc, LV_OBJ_FLAG_HIDDEN);
    }

    RunStateMachine();

    DrawBoat();
    DrawSpeedometer();
    DrawRoute();
    DrawDestinationCrosshair();

    // The background isn't an LVGL object, so make sure it's redrawn
    lv_obj_invalidate(m_screen);
}

void
UserInterface::MapScreen::RunStateMachine()
{
    auto before = m_state;

    auto zoom_mismatch = [this]() {
        return m_zoom_level == 2 && m_mode == Mode::kZoom4 ||
               m_zoom_level == 4 && m_mode == Mode::kZoom2;
    };

    do
    {
        before = m_state;

        switch (m_state)
        {
        case State::kMap:
            m_zoom_level = 1;

            if (m_parent.m_select_position)
            {
                m_mode = Mode::kZoom2;
                m_crosshair_position = m_parent.m_position;
                m_state = State::kInitialOverviewMap;
            }
            else if (m_parent.m_adjust_gps)
            {
                m_state = State::kAdjustGps;
            }
            else if (m_mode != Mode::kMap)
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

            if (m_parent.m_select_position)
            {
                m_mode = Mode::kZoom2;
                if (m_zoomed_out_map_tiles.empty())
                {
                    m_state = State::kSelectDestination;
                }
            }
            else if (m_zoomed_out_map_tiles.empty())
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
            else
            {
                const auto limit = 30 * m_zoom_level;

                if (m_parent.m_position.x < m_map_position_zoomed_out.x + limit ||
                    m_parent.m_position.y < m_map_position_zoomed_out.y + limit ||
                    m_parent.m_position.x >
                        m_map_position_zoomed_out.x + hal::kDisplayWidth * m_zoom_level - limit ||
                    m_parent.m_position.y >
                        m_map_position_zoomed_out.y + hal::kDisplayHeight * m_zoom_level - limit)
                {
                    // Boat outside the visible area
                    m_state = State::kInitialOverviewMap;
                }
            }
            break;

        case State::kSelectDestination:
            if (m_crosshair_position.x < m_map_position_zoomed_out.x + 60 ||
                m_crosshair_position.y < m_map_position_zoomed_out.y + 60 ||
                m_crosshair_position.x >
                    m_map_position_zoomed_out.x + hal::kDisplayWidth * m_zoom_level - 60 ||
                m_crosshair_position.y >
                    m_map_position_zoomed_out.y + hal::kDisplayHeight * m_zoom_level - 60)
            {
                // Boat outside the visible area
                m_state = State::kInitialOverviewMap;
            }

            if (m_parent.m_select_position == std::nullopt)
            {
                m_state = State::kDestinationSelected;
            }
            break;

        case State::kAdjustGps:
            m_zoom_level = 1;
            m_mode = Mode::kMap;

            if (m_parent.m_adjust_gps == false)
            {
                m_state = State::kGpsAdjusted;
            }

            break;

        case State::kGpsAdjusted:
            [[fallthrough]];

        case State::kDestinationSelected:
            m_mode = Mode::kMap;
            m_crosshair_position = Point {0, 0};
            m_show_route = true;
            lv_obj_remove_flag(m_boat, LV_OBJ_FLAG_HIDDEN);

            m_state = State::kMap;
            break;

        case State::kValueCount:
            break;
        }
    } while (m_state != before);
}

void
UserInterface::MapScreen::DrawBoat()
{
    int32_t x;
    int32_t y;

    if (m_state == State::kMap || m_state == State::kAdjustGps)
    {
        x = m_parent.m_position.x - m_map_position.x;
        y = m_parent.m_position.y - m_map_position.y;
    }
    else
    {
        x = (m_parent.m_position.x - m_map_position_zoomed_out.x) / m_zoom_level;
        y = (m_parent.m_position.y - m_map_position_zoomed_out.y) / m_zoom_level;
    }

    lv_obj_align(
        m_boat, LV_ALIGN_TOP_LEFT, x - m_boat_data->Width() / 2, y - m_boat_data->Height() / 2);
}

void
UserInterface::MapScreen::DrawDestinationCrosshair()
{
    auto x = (m_crosshair_position.x - m_map_position_zoomed_out.x) / m_zoom_level;
    auto y = (m_crosshair_position.y - m_map_position_zoomed_out.y) / m_zoom_level;

    lv_obj_align(m_crosshair,
                 LV_ALIGN_TOP_LEFT,
                 x - m_crosshair_data->Width() / 2,
                 y - m_crosshair_data->Height() / 2);
}

void
UserInterface::MapScreen::DrawSpeedometer()
{
    constexpr float max_angle = kSpeedometerMaxAngle;
    auto speed = std::clamp(m_parent.m_speed, 0.0f, static_cast<float>(kMaxKnots));

    auto angle = speed * max_angle / kMaxKnots;

    lv_arc_set_bg_angles(m_speedometer_arc, 0, angle);
}

bool
UserInterface::MapScreen::DrawsMapDirectly() const
{
    return m_state == State::kMap || m_state == State::kAdjustGps;
}

Point
UserInterface::MapScreen::ToDisplay(const Point& point) const
{
    if (m_state == State::kMap)
    {
        return {point.x - m_map_position.x, point.y - m_map_position.y};
    }

    return {(point.x - m_map_position_zoomed_out.x) / m_zoom_level,
            (point.y - m_map_position_zoomed_out.y) / m_zoom_level};
}

void
UserInterface::MapScreen::DrawBackground(uint16_t* dst)
{
    if (DrawsMapDirectly())
    {
        DrawMapTiles(dst, m_map_position);
    }
    else
    {
        memcpy(dst,
               m_static_map_buffer.get(),
               hal::kDisplayWidth * hal::kDisplayHeight * sizeof(uint16_t));
    }

    if (!m_show_route)
    {
        return;
    }

    const auto passed_color = lv_color_to_u16(lv_palette_main(LV_PALETTE_GREY));
    const auto remaining_color = lv_color_to_u16(lv_palette_main(LV_PALETTE_LIGHT_GREEN));

    // Passed first, so that the remaining route is drawn on top
    for (const auto& segment : m_route_segments)
    {
        if (segment.passed)
        {
            painter::DrawClippedLine<Point, 8>(dst, segment.from, segment.to, passed_color);
        }
    }
    for (const auto& segment : m_route_segments)
    {
        if (!segment.passed)
        {
            painter::DrawClippedLine<Point, 8>(dst, segment.from, segment.to, remaining_color);
        }
    }
}

void
UserInterface::MapScreen::DrawRoute()
{
    m_route_segments.clear();
    if (m_parent.m_route.empty())
    {
        return;
    }

    auto index = 1;
    auto route_iterator = RouteIterator(m_parent.m_route, m_parent.m_land_mask_row_size);
    auto last_point = route_iterator.Next();

    if (!last_point)
    {
        // Should be impossible, but anyway
        return;
    }

    while (auto cur_point = route_iterator.Next())
    {
        constexpr auto kThreshold = 2 * kPathFinderTileSize;

        // Mark the route as passed if the boat is close enough
        if (std::abs(m_parent.m_position.x - cur_point->x) < kThreshold &&
            std::abs(m_parent.m_position.y - cur_point->y) < kThreshold)
        {
            if (m_parent.m_passed_route_index < index)
            {
                m_parent.m_passed_route_index = index;
            }
        }

        // Clipped in place
        auto from = ToDisplay(*last_point);
        auto to = ToDisplay(*cur_point);
        if (cs::ClipLineToDisplay(from.x, from.y, to.x, to.y))
        {
            m_route_segments.push_back({from, to, index <= m_parent.m_passed_route_index});
        }

        index++;
        last_point = cur_point;
    }
}

void
UserInterface::MapScreen::DrawMapTiles(uint16_t* dst, const Point& position)
{
    auto x_remainder = position.x % kTileSize;
    auto y_remainder = position.y % kTileSize;
    auto num_tiles_x =
        (hal::kDisplayWidth + kTileSize - 1) / kTileSize + static_cast<int>(!!x_remainder);
    auto num_tiles_y =
        (hal::kDisplayHeight + kTileSize - 1) / kTileSize + static_cast<int>(!!y_remainder);

    auto start_x = position.x - x_remainder;
    auto start_y = position.y - y_remainder;

    // Blit all needed tiles
    for (auto y = 0; y < num_tiles_y; y++)
    {
        for (auto x = 0; x < num_tiles_x; x++)
        {
            auto tile = m_parent.m_tile_producer.LockTile(
                {start_x + x * kTileSize, start_y + y * kTileSize});
            if (tile)
            {
                painter::Blit(dst,
                              tile->GetImage(),
                              {x * kTileSize - x_remainder, y * kTileSize - y_remainder});
            }
        }
    }
}


Point
UserInterface::MapScreen::PositionToMapCenter(const Point& pixel_position) const
{
    auto x = pixel_position.x;
    auto y = pixel_position.y;

    const auto map_pixel_width = static_cast<int>(m_parent.m_tile_row_size * kTileSize);
    const auto map_pixel_height = static_cast<int>(m_parent.m_tile_rows * kTileSize);
    const auto max_x = std::max(0, map_pixel_width - hal::kDisplayWidth);
    const auto max_y = std::max(0, map_pixel_height - hal::kDisplayHeight);

    x = std::clamp(static_cast<int>(x - hal::kDisplayWidth / 2), 0, max_x);
    y = std::clamp(static_cast<int>(y - hal::kDisplayHeight / 2), 0, max_y);

    return {x, y};
}

void
UserInterface::MapScreen::PrepareInitialZoomedOutMap()
{
    // Free old tiles and fill with black
    m_zoomed_out_map_tiles.clear();
    memset(
        m_static_map_buffer.get(), 0, hal::kDisplayWidth * hal::kDisplayHeight * sizeof(uint16_t));

    // Align with the nearest tile
    auto aligned = Point {m_parent.m_position.x - m_parent.m_position.x % kTileSize,
                          m_parent.m_position.y - m_parent.m_position.y % kTileSize};

    if (m_parent.m_select_position)
    {
        aligned = Point {m_crosshair_position.x - m_crosshair_position.x % kTileSize,
                         m_crosshair_position.y - m_crosshair_position.y % kTileSize};
    }

    const auto offset_x =
        std::max(static_cast<int32_t>(0), aligned.x - (m_zoom_level * hal::kDisplayWidth) / 2);
    const auto offset_y =
        std::max(static_cast<int32_t>(0), aligned.y - (m_zoom_level * hal::kDisplayHeight) / 2);
    m_map_position_zoomed_out = Point {offset_x, offset_y};

    auto num_tiles_x = hal::kDisplayWidth / (kTileSize / m_zoom_level);
    auto num_tiles_y = hal::kDisplayHeight / (kTileSize / m_zoom_level);

    etl::vector<Point, kTileCacheSize> cached_tiles;

    for (auto y = 0; y < num_tiles_y; y++)
    {
        for (auto x = 0; x < num_tiles_x; x++)
        {
            auto at = Point {offset_x + x * kTileSize, offset_y + y * kTileSize};

            if (m_parent.m_tile_producer.IsCached(at) && cached_tiles.full() == false)
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

    m_parent.Awake();
}

void
UserInterface::MapScreen::FillZoomedOutMap()
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

    m_parent.Awake();
}

void
UserInterface::MapScreen::DrawZoomedTile(const Point& position)
{
    auto tile = m_parent.m_tile_producer.LockTile(position);
    if (tile)
    {
        auto dst = Point {position.x - m_map_position_zoomed_out.x,
                          position.y - m_map_position_zoomed_out.y};
        painter::ZoomedBlit(m_static_map_buffer.get(),
                            hal::kDisplayWidth,
                            tile->GetImage(),
                            m_zoom_level,
                            {dst.x / m_zoom_level, dst.y / m_zoom_level});
    }
}

void
UserInterface::MapScreen::OnInput(hal::IInput::Event event)
{
    if (m_state == State::kSelectDestination)
    {
        OnInputSelectDestination(event);
    }
    else if (m_state == State::kAdjustGps)
    {
        OnInputAdjustGps(event);
    }
    else
    {
        OnInputViewMap(event);
    }
}

void
UserInterface::MapScreen::OnInputViewMap(hal::IInput::Event event)
{
    auto mode = std::to_underlying(m_mode);
    switch (event.type)
    {
    case hal::IInput::EventType::kButtonDown:
        break;
    case hal::IInput::EventType::kButtonUp:
        m_parent.EnterMenu();
        break;
    case hal::IInput::EventType::kLeft:
        if (mode == 0)
        {
            mode = std::to_underlying(Mode::kValueCount);
        }
        mode--;
        break;
    case hal::IInput::EventType::kRight:
        mode++;
        break;
    default:
        break;
    }

    m_mode = static_cast<Mode>(mode % std::to_underlying(Mode::kValueCount));
}

void
UserInterface::MapScreen::OnInputSelectDestination(hal::IInput::Event event)
{
    auto x_diff = m_parent.m_position_select_vertical ? 0 : kPathFinderTileSize;
    auto y_diff = m_parent.m_position_select_vertical ? kPathFinderTileSize : 0;

    switch (event.type)
    {
    case hal::IInput::EventType::kButtonDown:
        m_select_timer = m_parent.StartTimer(1s, [this]() {
            m_parent.m_position_select_vertical = !m_parent.m_position_select_vertical;

            return std::nullopt;
        });
        break;
    case hal::IInput::EventType::kButtonUp:
        // Only select unless the timer has expired (otherwise it's a change of direction)
        if (m_parent.m_select_position && m_select_timer->IsExpired() == false)
        {
            if (m_parent.m_select_position == PositionSelection::kHome)
            {
                auto ps = m_parent.m_application_state.CheckoutPartialSnapshot<AS::configuration>();

                ps.GetWritableReference<AS::configuration>().home_position =
                    PointToLandIndex(m_crosshair_position, m_parent.m_land_mask_row_size);
            }
            else if (m_parent.m_select_position == PositionSelection::kNewRoute)
            {
                m_parent.m_route_service.RequestRoute(m_parent.m_position, m_crosshair_position);
            }

            m_parent.m_select_position = std::nullopt;
        }
        break;
    case hal::IInput::EventType::kLeft:
        m_crosshair_position =
            Point {m_crosshair_position.x - x_diff, m_crosshair_position.y - y_diff};
        break;
    case hal::IInput::EventType::kRight:
        m_crosshair_position =
            Point {m_crosshair_position.x + x_diff, m_crosshair_position.y + y_diff};
        break;
    default:
        break;
    }
}

void
UserInterface::MapScreen::OnInputAdjustGps(hal::IInput::Event event)
{
    auto ps = m_parent.m_application_state.CheckoutPartialSnapshot<AS::configuration>();
    auto& conf = ps.GetWritableReference<AS::configuration>();
    int8_t long_diff = m_parent.m_position_select_vertical ? 0 : 1;
    int8_t lat_diff = m_parent.m_position_select_vertical ? 1 : 0;

    switch (event.type)
    {
    case hal::IInput::EventType::kButtonDown:
        m_select_timer = m_parent.StartTimer(1s, [this]() {
            m_parent.m_position_select_vertical = !m_parent.m_position_select_vertical;

            return std::nullopt;
        });
        break;
    case hal::IInput::EventType::kButtonUp:
        // Only select unless the timer has expired (otherwise it's a change of direction)
        if (m_parent.m_adjust_gps && m_select_timer->IsExpired() == false)
        {
            m_parent.m_adjust_gps = false;
        }
        break;
    case hal::IInput::EventType::kLeft:
        conf.latitude_adjustment += -1 * lat_diff;
        conf.longitude_adjustment += -1 * long_diff;
        break;
    case hal::IInput::EventType::kRight:
        conf.latitude_adjustment += lat_diff;
        conf.longitude_adjustment += long_diff;
        break;
    default:
        break;
    }
}
