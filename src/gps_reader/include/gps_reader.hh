#pragma once

#include "base_thread.hh"
#include "gps_port.hh"

#include <etl/vector.h>
#include <array>
#include <atomic>

class GpsReader : public os::BaseThread
{
public:
    GpsReader();

    std::unique_ptr<IGpsPort> AttachListener();

private:
    class GpsPortImpl;

    std::optional<milliseconds> OnActivation() final;

    etl::vector<GpsPortImpl*, 8> m_listeners;
    std::array<std::atomic_bool, 8> m_stale_listeners;
};
