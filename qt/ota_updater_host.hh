#pragma once

#include "hal/i_ota_updater.hh"

class OtaUpdaterHost : public hal::IOtaUpdater
{
private:
    void Setup(std::function<void(uint8_t)> progress) final;

    uint8_t m_progress {0};
};