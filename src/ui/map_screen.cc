#include "map_screen.hh"

#include "boat.hh"
#include "cohen_sutherland.hh"
#include "painter.hh"

UserInterface::MapScreen::MapScreen(UserInterface& parent)
    : m_parent(parent)
    , m_boat_data(DecodePngMask(boat_data, 0))
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
    static lv_style_t style_line;
    lv_style_init(&style_line);
    lv_style_set_line_width(&style_line, 8);
    lv_style_set_line_color(&style_line, lv_palette_main(LV_PALETTE_LIGHT_GREEN));
    lv_style_set_line_rounded(&style_line, true);

    m_route_line = std::make_unique<RouteLine>(m_screen);

    lv_obj_add_style(m_route_line->lv_line, &style_line, 0);

    m_speedometer_arc = lv_arc_create(m_screen);
    lv_obj_set_size(m_speedometer_arc, lv_pct(100), lv_pct(100));
    lv_obj_remove_style(m_speedometer_arc, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(m_speedometer_arc, LV_OBJ_FLAG_CLICKABLE);
    //    lv_obj_set_style_arc_color(m_speedometer_arc,
    //                               lv_color_t {.red = 50, .blue = 155, .green = 255},
    //                               LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(m_speedometer_arc, 30, LV_PART_INDICATOR);
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

    lv_scale_set_total_tick_count(m_speedometer_scale, 41);
    lv_scale_set_major_tick_every(m_speedometer_scale, 5);

    lv_obj_set_style_length(m_speedometer_scale, 10, LV_PART_ITEMS);
    lv_obj_set_style_length(m_speedometer_scale, 20, LV_PART_INDICATOR);
    lv_obj_set_style_width(m_speedometer_scale, 2, LV_PART_ITEMS);
    lv_obj_set_style_width(m_speedometer_scale, 2, LV_PART_INDICATOR);
    lv_scale_set_range(m_speedometer_scale, 0, 40);

    lv_scale_set_angle_range(m_speedometer_scale, 270);
    lv_scale_set_rotation(m_speedometer_scale, 135);

    m_boat = lv_image_create(m_screen);
    lv_image_set_src(m_boat, &m_boat_data->lv_image_dsc);
    lv_obj_center(m_boat);
}

void
UserInterface::MapScreen::OnPosition(const GpsData& position)
{
    // Update the boat position in global pixel coordinates
    m_map_position = PositionToMapCenter(m_parent.m_position);

    lv_image_set_rotation(m_boat, position.heading * 10);
}

void
UserInterface::MapScreen::Activate()
{
    lv_screen_load(m_screen);
}

void
UserInterface::MapScreen::Update()
{
    if (m_parent.m_show_speedometer)
    {
    }
    else
    {
    }

    RunStateMachine();

    DrawBoat();
    DrawSpeedometer();
    DrawRoute();
}

void
UserInterface::MapScreen::RunStateMachine()
{
    auto before = m_state;

    auto zoom_mismatch = [this]() {
        return m_parent.m_zoom_level == 2 && m_parent.m_mode == Mode::kZoom4 ||
               m_parent.m_zoom_level == 4 && m_parent.m_mode == Mode::kZoom2;
    };

    do
    {
        switch (m_state)
        {
        case State::kMap:
            m_parent.m_zoom_level = 1;
            DrawMapTiles(m_map_position);

            if (m_parent.m_mode != Mode::kMap)
            {
                // Always go through this
                m_state = State::kInitialOverviewMap;
            }
            break;

        case State::kInitialOverviewMap:
            m_parent.m_zoom_level = m_parent.m_mode == Mode::kZoom2 ? 2 : 4;
            PrepareInitialZoomedOutMap();

            m_state = State::kFillOverviewMapTiles;

            break;
        case State::kFillOverviewMapTiles:
            FillZoomedOutMap();

            if (m_zoomed_out_map_tiles.empty())
            {
                m_state = State::kOverviewMap;
            }
            else if (m_parent.m_mode == Mode::kMap)
            {
                m_state = State::kMap;
            }
            else if (zoom_mismatch())
            {
                m_state = State::kInitialOverviewMap;
            }
            break;

        case State::kOverviewMap:
            if (m_parent.m_mode == Mode::kMap)
            {
                m_state = State::kMap;
            }
            else if (zoom_mismatch())
            {
                m_state = State::kInitialOverviewMap;
            }
            else if (m_parent.m_position.x < m_map_position_zoomed_out.x + 30 ||
                     m_parent.m_position.y < m_map_position_zoomed_out.y + 30 ||
                     m_parent.m_position.x > m_map_position_zoomed_out.x +
                                                 hal::kDisplayWidth * m_parent.m_zoom_level - 30 ||
                     m_parent.m_position.y > m_map_position_zoomed_out.y +
                                                 hal::kDisplayHeight * m_parent.m_zoom_level - 30)
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
        x = (m_parent.m_position.x - m_map_position_zoomed_out.x) / m_parent.m_zoom_level;
        y = (m_parent.m_position.y - m_map_position_zoomed_out.y) / m_parent.m_zoom_level;
    }

    lv_obj_align(
        m_boat, LV_ALIGN_TOP_LEFT, x - m_boat_data->Width() / 2, y - m_boat_data->Height() / 2);
}

void
UserInterface::MapScreen::DrawSpeedometer()
{
    constexpr auto max_speed = 40.0f;
    constexpr auto max_angle = 270.0f;
    auto speed = std::min(m_parent.m_speed, max_speed);

    auto angle = speed * max_angle / max_speed;

    lv_arc_set_bg_angles(m_speedometer_arc, 0, angle);
}

void
UserInterface::MapScreen::DrawRoute()
{
    m_route_line->points.clear();
    lv_line_set_points(m_route_line->lv_line, {}, 0);
    if (m_parent.m_route.empty())
    {
        return;
    }

    auto index = 0;
    auto route_iterator = RouteIterator(m_parent.m_route, m_parent.m_land_mask_row_size);
    auto last_point = route_iterator.Next();

    if (cs::PointClipsDisplay(last_point->x - m_map_position.x, last_point->y - m_map_position.y))
    {
        m_route_line->points.push_back(
            {last_point->x - m_map_position.x, last_point->y - m_map_position.y});
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

        if (!cs::LineClipsDisplay(last_point->x - m_map_position.x,
                                  last_point->y - m_map_position.y,
                                  cur_point->x - m_map_position.x,
                                  cur_point->y - m_map_position.y))
        {
            last_point = cur_point;
            index++;
            continue;
        }

        auto color = m_parent.m_passed_route_index && index < *m_parent.m_passed_route_index
                         ? 0x7BEF
                         : 0x07E0; // Green in RGB565

        if (m_state == State::kMap)
        {
            m_route_line->points.push_back(
                {cur_point->x - m_map_position.x, cur_point->y - m_map_position.y});
        }
        else
        {
            m_route_line->points.push_back(
                {(cur_point->x - m_map_position_zoomed_out.x) / m_parent.m_zoom_level,
                 (cur_point->y - m_map_position_zoomed_out.y) / m_parent.m_zoom_level});
        }

        last_point = cur_point;
        index++;
    }

    lv_line_set_points(
        m_route_line->lv_line, m_route_line->points.data(), m_route_line->points.size());
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

    const auto offset_x = std::max(static_cast<int32_t>(0),
                                   aligned.x - (m_parent.m_zoom_level * hal::kDisplayWidth) / 2);
    const auto offset_y = std::max(static_cast<int32_t>(0),
                                   aligned.y - (m_parent.m_zoom_level * hal::kDisplayHeight) / 2);
    m_map_position_zoomed_out = Point {offset_x, offset_y};

    auto num_tiles_x = hal::kDisplayWidth / (kTileSize / m_parent.m_zoom_level);
    auto num_tiles_y = hal::kDisplayHeight / (kTileSize / m_parent.m_zoom_level);

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
    const auto num_tiles_x = hal::kDisplayWidth / (kTileSize / m_parent.m_zoom_level);
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
                            tile->GetImage(),
                            m_parent.m_zoom_level,
                            {dst.x / m_parent.m_zoom_level, dst.y / m_parent.m_zoom_level});
    }
}
