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
    m_rotated_boat = painter::Rotate(*m_boat, m_boat_rotation, 0);

    input.AttachListener(this);
    m_gps_port->AwakeOn(GetSemaphore());
    m_route_listener->AwakeOn(GetSemaphore());
}

std::optional<milliseconds>
UserInterface::OnActivation()
{
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

    // Handle input
    hal::IInput::Event event;
    while (m_input_queue.pop(event))
    {
        auto state = std::to_underlying(m_state);

        switch (event.type)
        {
        case hal::IInput::EventType::kButtonDown:
            break;
        case hal::IInput::EventType::kButtonUp:
            m_show_speedometer = !m_show_speedometer;
            break;
        case hal::IInput::EventType::kLeft:
            state--;
            break;
        case hal::IInput::EventType::kRight:
            state++;
            break;
        default:
            break;
        }
        m_state = static_cast<State>(state % std::to_underlying(State::kValueCount));
    }


    RequestMapTiles();

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
UserInterface::RequestMapTiles()
{
    m_tiles.clear();

    auto x_remainder = m_map_position.x % kTileSize;
    auto y_remainder = m_map_position.y % kTileSize;
    auto num_tiles_x = (hal::kDisplayWidth + kTileSize - 1) / kTileSize + !!x_remainder;
    auto num_tiles_y = (hal::kDisplayHeight + kTileSize - 1) / kTileSize + !!y_remainder;

    auto start_x = m_map_position.x - x_remainder;
    auto start_y = m_map_position.y - y_remainder;

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
UserInterface::DrawMap()
{
    for (const auto& [tile, position] : m_tiles)
    {
        auto& image = static_cast<const ImageImpl&>(tile->GetImage());
        painter::Blit(m_frame_buffer, tile->GetImage(), {position.x, position.y});
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
