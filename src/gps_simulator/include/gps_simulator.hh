#pragma once

#include "base_thread.hh"
#include "hal/i_gps.hh"


class GpsSimulator : public hal::IGps, public os::BaseThread
{
public:
    GpsSimulator();

private:
    std::optional<milliseconds> OnActivation() final;

    hal::RawGpsData WaitForData(os::binary_semaphore& semaphore) final;

    GpsPosition m_current_position;
    float m_speed {0};
    float m_heading {0};
    os::binary_semaphore m_has_data_semaphore {0};
};
