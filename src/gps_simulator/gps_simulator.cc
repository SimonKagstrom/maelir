#include "gps_simulator.hh"


GpsSimulator::GpsSimulator()
{
    m_current_position = {59.51414995879205, 17.040945581855294};
//    m_current_position = {59.32296467190827, 17.78252273020107};
}

std::optional<milliseconds>
GpsSimulator::OnActivation()
{
    m_current_position.latitude -= 0.000005;
    m_current_position.longitude += 0.00002;
    m_heading += 1;

    if (m_heading > 360)
    {
        m_heading = 0;
    }

    m_has_data_semaphore.release();

    return 80ms;
}

hal::RawGpsData
GpsSimulator::WaitForData(os::binary_semaphore& semaphore)
{
    m_has_data_semaphore.acquire();

    semaphore.release();

    return {m_current_position, m_heading, m_speed};
}
