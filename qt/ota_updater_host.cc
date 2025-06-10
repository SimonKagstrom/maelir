#include "ota_updater_host.hh"

#include "time.hh"

OtaUpdaterHost::OtaUpdaterHost(bool updated)
    : m_updated(updated)
{
}


void
OtaUpdaterHost::Update(std::function<void(uint8_t)> progress)
{
    for (auto perc = 0; perc <= 100; ++perc)
    {
        progress(perc);
        os::Sleep(50ms);
    }
}


bool
OtaUpdaterHost::ApplicationHasBeenUpdated() const
{
    return m_updated;
}

void
OtaUpdaterHost::MarkApplicationAsValid()
{
    printf("OtaUpdater: MarkApplicationAsValid\n");
}

const char*
OtaUpdaterHost::GetSsid()
{
    return "Maelir";
}