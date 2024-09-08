#include "gps_simulator.hh"

#include "generated_tiles.hh"

GpsSimulator::GpsSimulator()
{
    m_current_position = {kCornerLatitude - 0.0481, kCornerLongitude + 0.090};
}

std::optional<milliseconds>
GpsSimulator::OnActivation()
{
    m_current_position.position.latitude += 0.00001;
    m_current_position.position.longitude += 0.00001;

    m_has_data_semaphore.release();

    return 50ms;
}

hal::RawGpsData
GpsSimulator::WaitForData(os::binary_semaphore& semaphore)
{
    m_has_data_semaphore.acquire();

    semaphore.release();

    return m_current_position;
}
