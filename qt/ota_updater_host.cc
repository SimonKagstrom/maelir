#include "ota_updater_host.hh"

#include "time.hh"

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
    return false;
}

void
OtaUpdaterHost::MarkApplicationAsValid()
{
    // NOP on the host
}
