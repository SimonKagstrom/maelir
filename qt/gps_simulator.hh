#pragma once

#include "hal/i_gps.hh"

#include <QObject>
#include <QTimer>

class GpsSimulator : public QObject, public hal::IGps
{
    Q_OBJECT
public:
    GpsSimulator();

private:
    GpsData WaitForData(os::binary_semaphore& semaphore) final;

    QTimer m_timer;
    GpsData m_current_position;
    os::binary_semaphore m_has_data_semaphore {0};
};
