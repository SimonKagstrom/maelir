#pragma once

#include "hal/i_ota_updater.hh"

class OtaUpdaterHost : public hal::IOtaUpdater
{
private:
    void Update(std::function<void(uint8_t)> progress) final;

    bool ApplicationHasBeenUpdated() const final;

    void MarkApplicationAsValid() final;

    uint8_t m_progress {0};
};