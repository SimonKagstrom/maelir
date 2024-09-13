#include "ui.hh"

#include "painter.hh"
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

    auto fb = m_display.GetFrameBuffer();
    painter::DrawAlphaLine(fb, {300, 300}, {320, 320}, 5, 0xF81F, 0);
    painter::DrawAlphaLine(fb, {220, 220}, {200, 200}, 10, 0x381F, 0);
    painter::DrawAlphaLine(fb, {240, 240}, {240, 220}, 10, 0x381F, 0);
    painter::DrawAlphaLine(fb, {300, 300}, {280, 280}, 3, 0xF21F, 128);
    painter::DrawAlphaLine(fb, {300, 300}, {300, 320}, 15, 0xc81F, 128);
    painter::DrawAlphaLine(fb, {300, 300}, {200, 300}, 7, 0x681F, 0);
    painter::DrawAlphaLine(fb, {300, 300}, {300, 200}, 6, 0x981F, 0);

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

    auto fb = m_display.GetFrameBuffer();

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
                painter::Blit(fb,
                              tile->GetImage(),
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

    painter::Blit(m_display.GetFrameBuffer(), m_boat, {x, y});
}
