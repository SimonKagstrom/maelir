#pragma once

#include <atomic>

struct ApplicationState
{
    std::atomic<bool> demo_mode {false};
    std::atomic<bool> gps_connected {false};
};
