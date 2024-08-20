#include "ui.hh"

#include "tile_utils.hh"

UserInterface::UserInterface(TileProducer& tile_producer,
                             hal::IDisplay& display,
                             std::unique_ptr<IGpsPort> gps_port)
    : BaseThread(1)
    , m_tile_producer(tile_producer)
    , m_display(display)
    , m_gps_port(std::move(gps_port))
{
    m_gps_port->AwakeOn(GetSemaphore());
}

std::optional<milliseconds>
UserInterface::OnActivation()
{
    if (auto position = m_gps_port->Poll())
    {
        auto [x, y] = PositionToPointCenteredAndClamped(*position);

        if (!NeedsRedraw(x, y))
        {
            return std::nullopt;
        }

        m_x = x;
        m_y = y;
        printf("POS now %f, %f -> pixels %d, %d -> tile %d, %d\n",
               position->latitude,
               position->longitude,
               x,
               y,
               m_x,
               m_y);
    }

    DrawMap();
    DrawBoat();

    m_display.Flip();

    return std::nullopt;
}

void
UserInterface::DrawMap()
{
    auto x_remainder = m_x % kTileSize;
    auto y_remainder = m_y % kTileSize;
    auto num_tiles_x = (hal::kDisplayWidth + kTileSize - 1) / kTileSize + !!x_remainder;
    auto num_tiles_y = (hal::kDisplayWidth + kTileSize - 1) / kTileSize + !!y_remainder;

    // Blit all needed tiles
    for (auto y = 0; y < num_tiles_y; y++)
    {
        for (auto x = 0; x < num_tiles_x; x++)
        {
            auto tile_x = (m_x / kTileSize + x) % kColumnSize;
            auto tile_y = (m_y / kTileSize + y) % kRowSize;

            auto tile = m_tile_producer.LockTile(m_x + x * kTileSize, m_y + y * kTileSize);
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
}

bool
UserInterface::NeedsRedraw(int32_t x, int32_t y) const
{
    return x != m_x || y != m_y;
}
