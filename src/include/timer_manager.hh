#pragma once

#include "semaphore.hh"
#include "time.hh"

#include <etl/vector.h>
#include <functional>
#include <optional>

namespace os
{

constexpr auto kMaxTimers = 8;

class ITimer
{
public:
    virtual ~ITimer() = default;

    virtual bool IsExpired() const = 0;

    virtual milliseconds TimeLeft() const = 0;
};

class TimerManager
{
public:
    TimerManager(os::binary_semaphore& semaphore);

    std::unique_ptr<ITimer> StartTimer(milliseconds timeout,
                                       std::function<std::optional<milliseconds>()> on_timeout);

    std::optional<milliseconds> Expire();

private:
    class TimerImpl;

    os::binary_semaphore& m_semaphore;

    etl::vector<TimerImpl*, kMaxTimers> m_timers;
    milliseconds m_last_expiery;
};

}; // namespace os
