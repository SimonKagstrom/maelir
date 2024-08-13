#pragma once

#include "base_thread.hh"
#include "gps_port.hh"
#include "hal/i_gps.hh"

#include <array>
#include <atomic>
#include <etl/vector.h>

class GpsReader : public os::BaseThread
{
public:
    GpsReader(hal::IGps& gps);

    std::unique_ptr<IGpsPort> AttachListener();

private:
    class GpsPortImpl;

    std::optional<milliseconds> OnActivation() final;

    hal::IGps& m_gps;
    etl::vector<GpsPortImpl*, 8> m_listeners;
    std::array<std::atomic_bool, 8> m_stale_listeners;
};
