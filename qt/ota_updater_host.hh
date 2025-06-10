#pragma once

#include "hal/i_ota_updater.hh"

class OtaUpdaterHost : public hal::IOtaUpdater
{
public:
    OtaUpdaterHost(bool updated);

private:
    void Update(std::function<void(uint8_t)> progress) final;

    bool ApplicationHasBeenUpdated() const final;

    void MarkApplicationAsValid() final;

    const char* GetSsid() final;

    const bool m_updated;
};
