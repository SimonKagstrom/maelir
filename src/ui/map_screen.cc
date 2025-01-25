#include "map_screen.hh"

#include "boat.hh"
#include "cohen_sutherland.hh"
#include "crosshair.hh"
#include "painter.hh"
#include "route_utils.hh"

#include <fmt/format.h>

constexpr auto kMaxKnots = 30;
constexpr auto kSpeedometerMaxAngle = 202;

UserInterface::MapScreen::MapScreen(UserInterface& parent)
    : m_parent(parent)
    , m_boat_data(DecodePngMask(boat_data, 0))
    , m_crosshair_data(DecodePngMask(crosshair_data, 0))
{
    m_static_map_buffer =
        std::make_unique<uint8_t[]>(hal::kDisplayWidth * hal::kDisplayHeight * sizeof(uint16_t));
    m_static_map_image = std::make_unique<Image>(
        std::span<const uint8_t> {m_static_map_buffer.get(),
                                  hal::kDisplayWidth * hal::kDisplayHeight * sizeof(uint16_t)},
        hal::kDisplayWidth,
        hal::kDisplayHeight);

    m_map_position = PositionToMapCenter(m_parent.m_position);


    m_background = lv_image_create(m_screen);

    lv_obj_move_background(m_background);
    lv_image_set_src(m_background, &m_static_map_image->lv_image_dsc);


    /* Create line style */
    static lv_style_t style_passed_line;
    static lv_style_t style_remaining_line;

    lv_style_init(&style_passed_line);
    lv_style_init(&style_remaining_line);

    lv_style_set_line_width(&style_passed_line, 8);
    lv_style_set_line_color(&style_passed_line, lv_palette_main(LV_PALETTE_GREY));
    lv_style_set_line_rounded(&style_passed_line, true);

    lv_style_set_line_width(&style_remaining_line, 8);
    lv_style_set_line_color(&style_remaining_line, lv_palette_main(LV_PALETTE_LIGHT_GREEN));
    lv_style_set_line_rounded(&style_remaining_line, true);

    m_route_line = std::make_unique<RouteLine>(m_screen);

    lv_obj_add_style(m_route_line->lv_remaining_line, &style_remaining_line, 0);
    lv_obj_add_style(m_route_line->lv_passed_line, &style_passed_line, 0);

    m_speedometer_arc = lv_arc_create(m_screen);
    lv_obj_set_size(m_speedometer_arc, lv_pct(100), lv_pct(100));
    lv_obj_remove_style(m_speedometer_arc, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(m_speedometer_arc, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_set_style_arc_width(m_speedometer_arc, 22, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(m_speedometer_arc, false, LV_PART_INDICATOR);
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
    auto state = m_parent.m_application_state.CheckoutReadonly();
    auto show_speedometer = state->show_speedometer;

    if (m_state == State::kSelectDestination)
    {
        lv_obj_add_flag(m_route_line->lv_passed_line, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(m_route_line->lv_remaining_line, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(m_crosshair, LV_OBJ_FLAG_HIDDEN);

        show_speedometer = false;
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

    if (m_parent.m_gps_position_valid == false && m_parent.m_calculating_route == false)
    {
        lv_obj_add_flag(m_indicators_shadow, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(m_indicators, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        char buf[32];

        snprintf(buf,
                 sizeof(buf),
                 "%s%s%s",
                 m_parent.m_gps_position_valid ? LV_SYMBOL_GPS : "",
                 m_parent.m_calculating_route ? " " : "",
                 m_parent.m_calculating_route ? LV_SYMBOL_LOOP : "");
        lv_label_set_text(m_indicators, buf);
        lv_label_set_text(m_indicators_shadow, buf);
        lv_obj_align_to(m_indicators_shadow, m_indicators, LV_ALIGN_TOP_LEFT, 2, 2);
        lv_obj_remove_flag(m_indicators_shadow, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(m_indicators, LV_OBJ_FLAG_HIDDEN);
    }

    RunStateMachine();

    DrawBoat();
    DrawSpeedometer();
    DrawRoute();
    DrawDestinationCrosshair();
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
            DrawMapTiles(m_map_position);

            if (m_parent.m_select_position)
            {
                m_mode = Mode::kZoom2;
                m_crosshair_position = m_parent.m_position;
                m_state = State::kInitialOverviewMap;
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
            else if (m_parent.m_position.x < m_map_position_zoomed_out.x + 30 ||
                     m_parent.m_position.y < m_map_position_zoomed_out.y + 30 ||
                     m_parent.m_position.x >
                         m_map_position_zoomed_out.x + hal::kDisplayWidth * m_zoom_level - 30 ||
                     m_parent.m_position.y >
                         m_map_position_zoomed_out.y + hal::kDisplayHeight * m_zoom_level - 30)
            {
                // Boat outside the visible area
                m_state = State::kInitialOverviewMap;
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

        case State::kDestinationSelected:
            m_mode = Mode::kMap;
            m_crosshair_position = Point {0, 0};
            lv_obj_remove_flag(m_route_line->lv_passed_line, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(m_route_line->lv_remaining_line, LV_OBJ_FLAG_HIDDEN);
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

    if (m_state == State::kMap)
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

void
UserInterface::MapScreen::AddRoutePoint(unsigned index, const Point& point) const
{
    auto has_passed_index = m_parent.m_passed_route_index && index < *m_parent.m_passed_route_index;
    auto passing_index = m_parent.m_passed_route_index && index == *m_parent.m_passed_route_index;

    lv_point_precise_t lv_point;

    if (m_state == State::kMap)
    {
        lv_point = {point.x - m_map_position.x, point.y - m_map_position.y};
    }
    else
    {
        lv_point = {(point.x - m_map_position_zoomed_out.x) / m_zoom_level,
                    (point.y - m_map_position_zoomed_out.y) / m_zoom_level};
    }

    if (has_passed_index)
    {
        m_route_line->passed_points.push_back(lv_point);
    }
    else if (passing_index)
    {
        m_route_line->passed_points.push_back(lv_point);
        m_route_line->remaining_points.push_back(lv_point);
    }
    else
    {
        m_route_line->remaining_points.push_back(lv_point);
    }
}

bool
UserInterface::MapScreen::PointClipsDisplay(const Point& point) const
{
    if (m_state == State::kMap)
    {
        return cs::PointClipsDisplay(point.x - m_map_position.x, point.y - m_map_position.y);
    }
    else
    {
        return cs::PointClipsDisplay((point.x - m_map_position_zoomed_out.x) / m_zoom_level,
                                     (point.y - m_map_position_zoomed_out.y) / m_zoom_level);
    }
}

bool
UserInterface::MapScreen::LineClipsDisplay(const Point& from, const Point& to) const
{
    if (m_state == State::kMap)
    {
        return cs::LineClipsDisplay(from.x - m_map_position.x,
                                    from.y - m_map_position.y,
                                    to.x - m_map_position.x,
                                    to.y - m_map_position.y);
    }
    else
    {
        return cs::LineClipsDisplay((from.x - m_map_position_zoomed_out.x) / m_zoom_level,
                                    (from.y - m_map_position_zoomed_out.y) / m_zoom_level,
                                    (to.x - m_map_position_zoomed_out.x) / m_zoom_level,
                                    (to.y - m_map_position_zoomed_out.y) / m_zoom_level);
    }
}

void
UserInterface::MapScreen::DrawRoute()
{
    m_route_line->passed_points.clear();
    m_route_line->remaining_points.clear();
    lv_line_set_points(m_route_line->lv_passed_line, {}, 0);
    lv_line_set_points(m_route_line->lv_remaining_line, {}, 0);
    if (m_parent.m_route.empty())
    {
        return;
    }

    auto index = 0;
    auto route_iterator = RouteIterator(m_parent.m_route, m_parent.m_land_mask_row_size);
    auto last_point = route_iterator.Next();

    if (!last_point)
    {
        // Should be impossible, but anyway
        return;
    }

    while (auto cur_point = route_iterator.Next())
    {
        constexpr auto kThreshold = 3 * kPathFinderTileSize;

        // Mark the route as passed if the boat is close enough
        if (std::abs(m_parent.m_position.x - cur_point->x) < kThreshold &&
            std::abs(m_parent.m_position.y - cur_point->y) < kThreshold)
        {
            if (!m_parent.m_passed_route_index || *m_parent.m_passed_route_index < index)
            {
                m_parent.m_passed_route_index = index;
            }
        }

        if (!LineClipsDisplay(*last_point, *cur_point))
        {
            last_point = cur_point;
            index++;
            continue;
        }
        AddRoutePoint(index++, *last_point);
        AddRoutePoint(index++, *cur_point);

        last_point = cur_point;
    }

    lv_line_set_points(m_route_line->lv_passed_line,
                       m_route_line->passed_points.data(),
                       m_route_line->passed_points.size());
    lv_line_set_points(m_route_line->lv_remaining_line,
                       m_route_line->remaining_points.data(),
                       m_route_line->remaining_points.size());
}

void
UserInterface::MapScreen::DrawMapTiles(const Point& position)
{
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
            auto tile = m_parent.m_tile_producer.LockTile(
                {start_x + x * kTileSize, start_y + y * kTileSize});
            if (tile)
            {
                painter::Blit(reinterpret_cast<uint16_t*>(m_static_map_buffer.get()),
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

    x = std::clamp(static_cast<int>(x - hal::kDisplayWidth / 2),
                   0,
                   static_cast<int>(m_parent.m_tile_row_size) * kTileSize - hal::kDisplayWidth);
    y = std::clamp(static_cast<int>(y - hal::kDisplayHeight / 2),
                   0,
                   static_cast<int>(m_parent.m_tile_row_size) * kTileSize - hal::kDisplayHeight);

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

    constexpr auto kVisibleTiles = hal::kDisplayWidth / kTileSize;
    etl::vector<Point, kVisibleTiles * kVisibleTiles> cached_tiles;

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
        painter::ZoomedBlit(reinterpret_cast<uint16_t*>(m_static_map_buffer.get()),
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
    printf("Event: %d. State now %d\n", (int)event.type, (int)m_mode);
}

void
UserInterface::MapScreen::OnInputSelectDestination(hal::IInput::Event event)
{
    auto is_vertical = m_parent.m_input.GetState().IsActive(hal::IInput::StateType::kSwitchUp);
    auto x_diff = is_vertical ? 0 : kPathFinderTileSize;
    auto y_diff = is_vertical ? kPathFinderTileSize : 0;

    switch (event.type)
    {
    case hal::IInput::EventType::kButtonDown:
        break;
    case hal::IInput::EventType::kButtonUp:
        if (m_parent.m_select_position)
        {
            if (m_parent.m_select_position == PositionSelection::kHome)
            {
                m_parent.m_application_state.Checkout()->home_position =
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
