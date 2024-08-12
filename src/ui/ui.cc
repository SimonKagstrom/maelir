#include "ui.hh"

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
    // This is TMP!
    auto t0 = m_tile_producer.LockTile(0, 0);
    auto t1 = m_tile_producer.LockTile(240, 0);
    auto t11 = m_tile_producer.LockTile(0, 240);
    auto t12 = m_tile_producer.LockTile(240, 240);

    if (t0 && t1 && t11 && t12)
    {
        m_display.Blit(t0->GetImage(), Rect {0, 0});
        m_display.Blit(t1->GetImage(), Rect {240, 0});
        m_display.Blit(t11->GetImage(), Rect {0, 240});
        m_display.Blit(t12->GetImage(), Rect {240, 240});
    }
    m_display.Flip();

    return std::nullopt;
}
