#include "ui.hh"

#include "boat.hh"
#include "painter.hh"
#include "route_iterator.hh"
#include "route_utils.hh"
#include "speed_dial.hh"
#include "speedometer.hh"
#include "time.hh"

#include <cmath>
#include <numbers>

UserInterface::UserInterface(const MapMetadata& metadata,
                             TileProducer& tile_producer,
                             hal::IDisplay& display,
                             hal::IInput& input,
                             std::unique_ptr<IGpsPort> gps_port,
                             std::unique_ptr<IRouteListener> route_listener)
    : m_tile_rows(metadata.tile_row_size)
    , m_tile_columns(metadata.tile_column_size)
    , m_land_mask_rows(metadata.land_mask_rows)
    , m_land_mask_row_size(metadata.land_mask_row_size)
    , m_tile_producer(tile_producer)
    , m_display(display)
    , m_gps_port(std::move(gps_port))
    , m_route_listener(std::move(route_listener))
    , m_boat(DecodePng(boat_data))
    , m_speed_dial(DecodePng(speed_dial_data))
    , m_speedometer(DecodePng(speedometer_data))
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
            break;
        case hal::IInput::EventType::kButtonUp:
            m_show_speedometer = !m_show_speedometer;
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

    m_display.Flip();
    // Now invalid
    m_frame_buffer = nullptr;

    return std::nullopt;
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
                m_tiles.push_back(
                    {std::move(tile), {x * kTileSize - x_remainder, y * kTileSize - y_remainder}});
            }
        }
    }
}

void
UserInterface::PrepareInitialZoomedOutMap()
{
    // Free old tiles and fill with black
    m_tiles.clear();
    m_zoomed_out_map_tiles.clear();
    memset(
        m_static_map_buffer.get(), 0, hal::kDisplayWidth * hal::kDisplayHeight * sizeof(uint16_t));

    // Align with the nearest tile
    auto aligned = Point {m_map_position.x - m_map_position.x % kTileSize,
                          m_map_position.y - m_map_position.y % kTileSize};

    auto offset_x =
        std::max(static_cast<int32_t>(0), aligned.x - (m_zoom_level * hal::kDisplayWidth) / 2);
    auto offset_y =
        std::max(static_cast<int32_t>(0), aligned.y - (m_zoom_level * hal::kDisplayHeight) / 2);
    m_map_position_zoomed_out = Point {offset_x, offset_y};

    auto num_tiles_x = hal::kDisplayWidth / (kTileSize / m_zoom_level);
    auto num_tiles_y = hal::kDisplayHeight / (kTileSize / m_zoom_level);

    for (auto y = 0; y < num_tiles_y; y++)
    {
        for (auto x = 0; x < num_tiles_x; x++)
        {
            m_zoomed_out_map_tiles.push_back({offset_x + x * kTileSize, offset_y + y * kTileSize});
        }
    }

    for (const auto& position : m_zoomed_out_map_tiles)
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

    return;
    RequestMapTiles(aligned);
    for (const auto& [tile, position] : m_tiles)
    {
        auto& image = static_cast<const ImageImpl&>(tile->GetImage());

        auto dst = Point {position.x - offset_x, position.y - offset_y};
        painter::ZoomedBlit(m_static_map_buffer.get(),
                            tile->GetImage(),
                            m_zoom_level,
                            {dst.x / m_zoom_level + hal::kDisplayWidth / 2,
                             dst.y / m_zoom_level + hal::kDisplayWidth / 2});
    }
}

void
UserInterface::RunStateMachine()
{
    auto before = m_state;

    auto zoom_mismatch = [this]() {
        return m_zoom_level == 2 && m_mode == Mode::kZoom3 ||
               m_zoom_level == 3 && m_mode == Mode::kZoom2;
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
            m_zoom_level = m_mode == Mode::kZoom2 ? 2 : 3;
            PrepareInitialZoomedOutMap();

            m_state = State::kFillOverviewMapTiles;

            break;
        case State::kFillOverviewMapTiles:
            if (m_mode == Mode::kMap)
            {
                m_state = State::kMap;
            }
            else if (m_mode == Mode::kGlobalMap)
            {
                m_state = State::kGlobalMap;
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
            else if (m_mode == Mode::kGlobalMap)
            {
                m_state = State::kGlobalMap;
            }
            if (zoom_mismatch())
            {
                m_state = State::kInitialOverviewMap;
            }
            break;
        case State::kGlobalMap:
            if (m_mode != Mode::kGlobalMap)
            {
                // Always go via the map
                m_state = State::kMap;
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
            auto& image = static_cast<const ImageImpl&>(tile->GetImage());
            painter::Blit(m_frame_buffer, tile->GetImage(), {position.x, position.y});
        }
    }
    else
    {
        painter::Blit(m_frame_buffer, *m_static_map_image, {0, 0});
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

        painter::DrawAlphaLine(m_frame_buffer,
                               {last_point->x - m_map_position.x, last_point->y - m_map_position.y},
                               {cur_point->x - m_map_position.x, cur_point->y - m_map_position.y},
                               8,
                               color,
                               128);

        last_point = cur_point;
        index++;
    }
}

void
UserInterface::DrawBoat()
{
    auto x = m_position.x - m_map_position.x - m_rotated_boat.width / 2;
    auto y = m_position.y - m_map_position.y - m_rotated_boat.height / 2;

    painter::MaskBlit(m_frame_buffer, m_rotated_boat, {x, y});
}

void
UserInterface::DrawSpeedometer()
{
    constexpr float kAngle0Knots = 130 * std::numbers::pi_v<float> / 180;
    constexpr float kAngle35Knots = 310 * std::numbers::pi_v<float> / 180;
    constexpr auto kCircleWidth = 536;
    constexpr auto kCentre = Point {hal::kDisplayWidth / 2, hal::kDisplayHeight / 2};

    painter::MaskBlit(m_frame_buffer, *m_speedometer, {0, 0});

    float angle = kAngle0Knots + (kAngle35Knots - kAngle0Knots) * m_speed / 35;
    int32_t x = kCentre.x + (kCircleWidth + m_speed_dial->width) * cosf(angle) / 2;
    int32_t y = kCentre.y + (kCircleWidth + m_speed_dial->height) * sinf(angle) / 2;

    painter::MaskBlit(m_frame_buffer,
                      *m_speed_dial,
                      Rect {x - m_speed_dial->width / 2, y - m_speed_dial->height / 2});
}

Point
UserInterface::PositionToMapCenter(const auto& pixel_position) const
{
    auto x = pixel_position.x;
    auto y = pixel_position.y;

    x = std::clamp(static_cast<int>(x - hal::kDisplayWidth / 2),
                   0,
                   static_cast<int>(m_tile_rows) * kTileSize - hal::kDisplayWidth);
    y = std::clamp(static_cast<int>(y - hal::kDisplayHeight / 2),
                   0,
                   static_cast<int>(m_tile_columns) * kTileSize - hal::kDisplayHeight);

    return {x, y};
}
