#include "gps_simulator.hh"

#include "tile.hh"

GpsSimulator::GpsSimulator()
{
    m_current_position = {kCornerLatitude, kCornerLongitude};

    m_timer.setInterval(50);
    m_timer.setSingleShot(false);
    connect(&m_timer, &QTimer::timeout, this, [this]() {
        m_current_position.latitude += 0.00002;
        m_current_position.longitude += 0.00002;

        m_has_data_semaphore.release();
    });
    m_timer.start();
}

GpsData
GpsSimulator::WaitForData(os::binary_semaphore& semaphore)
{
    m_has_data_semaphore.acquire();
    semaphore.release();

    return m_current_position;
}
