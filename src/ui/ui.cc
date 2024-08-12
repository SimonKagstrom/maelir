#include "ui.hh"

UserInterface::UserInterface(TileProducer& tile_producer, std::unique_ptr<IGpsPort> gps_port)
    : m_tile_producer(tile_producer)
    , m_gps_port(std::move(gps_port))
{
}

std::optional<milliseconds>
UserInterface::OnActivation()
{
    return std::nullopt;
}
