#include "ui.hh"

Ui::Ui(TileProducer& tile_producer) :
    m_tile_producer(tile_producer)
{
}

std::optional<milliseconds>
Ui::OnActivation()
{

    return std::nullopt;
}
