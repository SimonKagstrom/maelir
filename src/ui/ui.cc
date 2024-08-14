#include "ui.hh"

#include "tile_utils.hh"

constexpr auto kDisplayWidth = 480;
constexpr auto kDisplayHeight = 480;

UserInterface::UserInterface(TileProducer& tile_producer,
                             hal::IDisplay& display,
                             std::unique_ptr<IGpsPort> gps_port)
    : m_tile_producer(tile_producer)
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
        auto [x, y] = PositionToPoint(*position);
        m_x = x;
        m_y = y;
    }

    auto x_remainder = m_x % kTileSize;
    auto y_remainder = m_y % kTileSize;
    auto num_tiles_x = (kDisplayWidth + kTileSize - 1) / kTileSize + !!x_remainder;
    auto num_tiles_y = (kDisplayWidth + kTileSize - 1) / kTileSize + !!y_remainder;

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

    m_display.Flip();

    return std::nullopt;
}
