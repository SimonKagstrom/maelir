#include "gps_simulator.hh"

#include "generated_tiles.hh"

GpsSimulator::GpsSimulator()
{
    m_current_position = {59.51414995879205, 17.040945581855294};
}

std::optional<milliseconds>
GpsSimulator::OnActivation()
{
    m_current_position.position.latitude -= 0.00001;
    m_current_position.position.longitude += 0.00002;

    m_has_data_semaphore.release();

    return 40ms;
}

hal::RawGpsData
GpsSimulator::WaitForData(os::binary_semaphore& semaphore)
{
    m_has_data_semaphore.acquire();

    semaphore.release();

    return m_current_position;
}
