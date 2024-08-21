#include "gps_simulator.hh"

#include "tile.hh"
#include "tile_utils.hh"

GpsSimulator::GpsSimulator()
{
    m_current_position = {kCornerLatitude + 0.0481, kCornerLongitude + 0.090};
}

std::optional<milliseconds>
GpsSimulator::OnActivation()
{
    m_current_position.latitude += 0.00001;
    m_current_position.longitude += 0.00001;

    m_has_data_semaphore.release();

    return 50ms;
}

GpsData
GpsSimulator::WaitForData(os::binary_semaphore& semaphore)
{
    m_has_data_semaphore.acquire();

    auto [x, y] = PositionToPoint(m_current_position);

    semaphore.release();

    return m_current_position;
}
