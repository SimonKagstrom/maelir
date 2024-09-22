#include "ui.hh"

#include "generated_land_mask.hh"
#include "painter.hh"
#include "route_iterator.hh"
#include "route_utils.hh"
#include "tile_utils.hh"
#include "time.hh"

UserInterface::UserInterface(TileProducer& tile_producer,
                             hal::IDisplay& display,
                             std::unique_ptr<IGpsPort> gps_port,
                             std::unique_ptr<IRouteListener> route_listener)
    : m_tile_producer(tile_producer)
    , m_display(display)
    , m_gps_port(std::move(gps_port))
    , m_route_listener(std::move(route_listener))
    , m_boat(m_boat_pixels, 5, 5)
{
    for (auto& pixel : m_boat_pixels)
    {
        // Magenta in rgb565
        pixel = 0xF81F;
    }

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
    }

    if (auto route = m_route_listener->Poll())
    {
        if (route->type == IRouteListener::EventType::kRouteReady)
        {
            m_current_route = route->route;
        }
        else
        {
            m_current_route = {};
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
    os::Sleep(10ms);

    return std::nullopt;
}

void
UserInterface::RequestMapTiles()
{
    m_tiles.clear();

    auto x_remainder = m_map_x % kTileSize;
    auto y_remainder = m_map_y % kTileSize;
    auto num_tiles_x = (hal::kDisplayWidth + kTileSize - 1) / kTileSize + !!x_remainder;
    auto num_tiles_y = (hal::kDisplayWidth + kTileSize - 1) / kTileSize + !!y_remainder;

    // Blit all needed tiles
    for (auto y = 0; y < num_tiles_y; y++)
    {
        for (auto x = 0; x < num_tiles_x; x++)
        {
            auto tile_x = (m_map_x / kTileSize + x) % kColumnSize;
            auto tile_y = (m_map_y / kTileSize + y) % kRowSize;

            auto tile = m_tile_producer.LockTile(m_map_x + x * kTileSize, m_map_y + y * kTileSize);
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
    if (m_current_route.empty())
    {
        return;
    }

    auto route_iterator =
        RouteIterator(m_current_route,
                      PointToLandIndex({0, 0}, kLandMaskRows),
                      PointToLandIndex({kLandMaskRowSize, kLandMaskRows}, kLandMaskRowSize),
                      kLandMaskRows);

    auto last = route_iterator.Next();
    while (auto cur = route_iterator.Next())
    {
        auto index = *cur;

        auto last_point = LandIndexToPoint(*last, kLandMaskRowSize);
        auto cur_point = LandIndexToPoint(index, kLandMaskRowSize);

        painter::DrawAlphaLine(m_frame_buffer,
                               {last_point.x + kPathFinderTileSize / 2 - m_map_x,
                                last_point.y + kPathFinderTileSize / 2 - m_map_y},
                               {cur_point.x + kPathFinderTileSize / 2 - m_map_x,
                                cur_point.y + kPathFinderTileSize / 2 - m_map_y},
                               8,
                               // Green in RGB565
                               0x07E0,
                               128);

        last = cur;
    }
}

void
UserInterface::DrawBoat()
{
    auto x = m_x - m_map_x;
    auto y = m_y - m_map_y;

    painter::Blit(m_frame_buffer, m_boat, {x, y});
}
