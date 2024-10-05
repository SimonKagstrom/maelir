#include "ui.hh"

#include "boat.hh"
#include "painter.hh"
#include "route_iterator.hh"
#include "route_utils.hh"
#include "time.hh"


UserInterface::UserInterface(const MapMetadata& metadata,
                             TileProducer& tile_producer,
                             hal::IDisplay& display,
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
    , m_rotated_boat(*m_boat)
{
    assert(m_boat);

    // Data for the rotated boat
    auto size = static_cast<size_t>(m_boat->width * 1.5f * m_boat->height * 1.5f);
    m_boat_rotation_buffer = std::make_unique<uint16_t[]>(size);
    memset(m_boat_rotation_buffer.get(), 0, size * sizeof(uint16_t));
    m_boat_rotation = {m_boat_rotation_buffer.get(), size};
    m_rotated_boat = painter::Rotate(*m_boat, m_boat_rotation, 0);

    m_gps_port->AwakeOn(GetSemaphore());
    m_route_listener->AwakeOn(GetSemaphore());
}

std::optional<milliseconds>
UserInterface::OnActivation()
{
    if (auto position = m_gps_port->Poll())
    {
        auto [map_x, map_y] = PositionToMapCenter(position->pixel_position);

        // Update the boat position in global pixel coordinates
        m_x = position->pixel_position.x;
        m_y = position->pixel_position.y;

        m_map_x = map_x;
        m_map_y = map_y;

        m_rotated_boat = painter::Rotate(*m_boat, m_boat_rotation, position->heading);
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
        }
    }

    RequestMapTiles();

    // Can potentially have to wait
    m_frame_buffer = m_display.GetFrameBuffer();

    DrawMap();
    DrawRoute();
    DrawBoat();

    m_display.Flip();
    // Now invalid
    m_frame_buffer = nullptr;

    return std::nullopt;
}

void
UserInterface::RequestMapTiles()
{
    m_tiles.clear();

    auto x_remainder = m_map_x % kTileSize;
    auto y_remainder = m_map_y % kTileSize;
    auto num_tiles_x = (hal::kDisplayWidth + kTileSize - 1) / kTileSize + !!x_remainder;
    auto num_tiles_y = (hal::kDisplayHeight + kTileSize - 1) / kTileSize + !!y_remainder;

    auto start_x = m_map_x - x_remainder;
    auto start_y = m_map_y - y_remainder;

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

    auto route_iterator = RouteIterator(m_route, m_land_mask_row_size);

    auto last_point = route_iterator.Next();
    while (auto cur_point = route_iterator.Next())
    {
        painter::DrawAlphaLine(m_frame_buffer,
                               {last_point->x - m_map_x, last_point->y - m_map_y},
                               {cur_point->x - m_map_x, cur_point->y - m_map_y},
                               8,
                               // Green in RGB565
                               0x07E0,
                               128);

        last_point = cur_point;
    }
}

void
UserInterface::DrawBoat()
{
    auto x = m_x - m_map_x - m_rotated_boat.width / 2;
    auto y = m_y - m_map_y - m_rotated_boat.height / 2;

    painter::MaskBlit(m_frame_buffer, m_rotated_boat, {x, y});
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
