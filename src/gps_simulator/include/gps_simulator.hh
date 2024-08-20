#pragma once

#include "base_thread.hh"
#include "hal/i_gps.hh"


class GpsSimulator : public hal::IGps, public os::BaseThread
{
public:
    GpsSimulator();

private:
    std::optional<milliseconds> OnActivation() final;

    GpsData WaitForData(os::binary_semaphore& semaphore) final;

    GpsData m_current_position;
    os::binary_semaphore m_has_data_semaphore {0};
};