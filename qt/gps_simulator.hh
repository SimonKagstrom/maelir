#pragma once

#include "hal/i_gps.hh"

class GpsSimulator : public hal::IGps
{
private:
    GpsData WaitForData(std::binary_semaphore& semaphore) final;
};
