#include "gps_listener.hh"

GpsListener::GpsListener(hal::IGps& gps)
    : m_gps(gps)
{
}

void
GpsListener::AttachListener(std::function<void(hal::RawGpsData&)> on_data)
{
    m_on_data = std::move(on_data);
}

std::optional<milliseconds>
GpsListener::OnActivation()
{
    auto data = m_gps.WaitForData(GetSemaphore());

    if (data)
    {
        m_on_data(*data);
    }

    return std::nullopt;
}
