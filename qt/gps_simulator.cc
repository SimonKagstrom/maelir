#include "gps_simulator.hh"
#include "tile.hh"

GpsData GpsSimulator::WaitForData(std::binary_semaphore& semaphore)
{
    semaphore.release();
    return {kCornerLatitude, kCornerLongitude};
}
