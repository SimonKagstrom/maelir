#pragma once

#include "semaphore.hh"
#include "time.hh"

#include <array>
#include <etl/bitset.h>
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

using TimerHandle = std::unique_ptr<ITimer>;

class TimerManager
{
public:
    TimerManager(os::binary_semaphore& semaphore);

    TimerHandle StartTimer(milliseconds timeout,
                           std::function<std::optional<milliseconds>()> on_timeout);

    std::optional<milliseconds> Expire();

private:
    class TimerImpl;

    struct Entry
    {
        milliseconds timeout;
        std::function<std::optional<milliseconds>()> on_timeout;
        TimerImpl* cookie;
        bool expired;
    };

    Entry& EntryAt(uint8_t index)
    {
        return m_timers[index];
    }

    void BumpTime();

    void SortActiveTimers();
    milliseconds ActivatePendingTimers();
    void RemoveDeletedTimers();

    std::array<Entry, kMaxTimers> m_timers {};
    etl::vector<uint8_t, kMaxTimers> m_active_timers;

    milliseconds m_last_expiery;


    os::binary_semaphore& m_semaphore;

    etl::bitset<kMaxTimers, uint8_t> m_pending_removals;
    etl::bitset<kMaxTimers, uint8_t> m_pending_additions;
    etl::bitset<kMaxTimers, uint8_t> m_free_timers;
    bool m_in_expire {false};
};

}; // namespace os
