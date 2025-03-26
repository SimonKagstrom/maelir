#pragma once

#include "base_thread.hh"
#include "hal/i_gps.hh"

class GpsListener : public os::BaseThread
{
public:
    GpsListener(hal::IGps& gps);

    // TODO: Sloppy, should return a cookie etc
    void AttachListener(std::function<void(hal::RawGpsData&)> on_data);

private:
    std::optional<milliseconds> OnActivation() final;

    hal::IGps& m_gps;
    std::function<void(hal::RawGpsData&)> m_on_data {[](auto) {}};
};
