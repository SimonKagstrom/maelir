#include "ui.hh"

#include "tile_utils.hh"
#include "time.hh"


UserInterface::UserInterface(TileProducer& tile_producer,
                             hal::IDisplay& display,
                             std::unique_ptr<IGpsPort> gps_port)
    : m_tile_producer(tile_producer)
    , m_display(display)
    , m_gps_port(std::move(gps_port))
    , m_boat(m_boat_pixels, 5, 5)
{
    for (auto& pixel : m_boat_pixels)
    {
        // Magenta in rgb565
        pixel = 0xF81F;
    }

    m_x = 2400;
    m_map_x = 1680;

    m_gps_port->AwakeOn(GetSemaphore());
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
        printf("POS now %f, %f -> b %d,%d M %d,%d (out of max %d,%d)\n",
               position->position.latitude,
               position->position.longitude,
               m_x,
               m_y,
               m_map_x,
               m_map_y,
               kTileSize * kRowSize,
               kTileSize * kColumnSize);
    }

    DrawMap();
    DrawBoat();

    m_display.Flip();
    os::Sleep(10ms);

    return std::nullopt;
}

void
UserInterface::DrawMap()
{
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
                auto& image = static_cast<const ImageImpl&>(tile->GetImage());
                m_display.Blit(tile->GetImage(),
                               {x * kTileSize - x_remainder, y * kTileSize - y_remainder});
            }
        }
    }
}

void
UserInterface::DrawBoat()
{
    auto x = m_x - m_map_x;
    auto y = m_y - m_map_y;

    m_display.Blit(m_boat, {x, y});
}
