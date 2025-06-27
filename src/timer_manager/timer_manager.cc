#include "timer_manager.hh"

using namespace os;

constexpr auto kForever = milliseconds::max();
constexpr uint8_t kDetachedTimer = 0xff;

class TimerManager::TimerImpl : public ITimer
{
public:
    TimerImpl(TimerManager& manager, uint8_t entry_index)
        : m_manager(manager)
        , m_entry_index(entry_index)
    {
    }

    ~TimerImpl()
    {
        if (!IsExpired())
        {
            Detach();
        }
    }

    // Detach an expired timer (allow reusing it's index)
    void Detach()
    {
        m_manager.EntryAt(m_entry_index).cookie = nullptr;
        m_manager.m_pending_removals[m_entry_index] = true;

        m_entry_index = kDetachedTimer;
    }

private:
    bool IsExpired() const final
    {
        return m_entry_index == kDetachedTimer;
    }

    milliseconds TimeLeft() const final
    {
        if (IsExpired())
        {
            return 0ms;
        }

        return m_manager.EntryAt(m_entry_index).timeout;
    }

    TimerManager& m_manager;
    uint8_t m_entry_index;
};


TimerManager::TimerManager(os::binary_semaphore& semaphore)
    : m_semaphore(semaphore)
    , m_last_expiery(os::GetTimeStamp())
{
    m_free_timers.set();
}

void
TimerManager::SortActiveTimers()
{
    std::ranges::sort(m_active_timers, [this](auto a, auto b) {
        const auto& timer_a = m_timers[a];
        const auto& timer_b = m_timers[b];

        return timer_a.timeout < timer_b.timeout;
    });
}

TimerHandle
TimerManager::StartTimer(milliseconds timeout,
                         std::function<std::optional<milliseconds>()> on_timeout)
{
    if (m_free_timers.none())
    {
        return nullptr;
    }

    auto index = static_cast<uint8_t>(m_free_timers.find_first(true));
    m_free_timers[index] = false;

    if (m_in_expire)
    {
        // Starting the timer from the callback of another: Add to pending for later processing
        m_pending_additions[index] = true;
    }
    else
    {
        BumpTime();
        m_active_timers.push_back(index);
    }

    auto cookie = new TimerImpl(*this, index);

    auto& timer = m_timers[index];

    timer.timeout = timeout;
    timer.on_timeout = std::move(on_timeout);
    timer.cookie = cookie;

    return TimerHandle(cookie);
}

milliseconds
TimerManager::ActivatePendingTimers(milliseconds next_wakeup)
{
    if (m_pending_additions.none())
    {
        return next_wakeup;
    }

    for (auto index = m_pending_additions.find_first(true); index != m_pending_additions.npos;
         index = m_pending_additions.find_next(true, index + 1))
    {
        m_active_timers.push_back(index);
        const auto& timer = m_timers[index];

        next_wakeup = std::min(next_wakeup, timer.timeout);
    }
    m_pending_additions.reset();

    return next_wakeup;
}

void
TimerManager::RemoveDeletedTimers()
{
    for (auto index = m_pending_removals.find_first(true); index != m_pending_removals.npos;
         index = m_pending_removals.find_next(true, index + 1))
    {
        if (auto it = std::ranges::find(m_active_timers, index); it != m_active_timers.end())
        {
            m_active_timers.erase(it);
        }

        m_free_timers[index] = true;
    }

    m_pending_removals.reset();
}

void
TimerManager::BumpTime()
{
    auto now = os::GetTimeStamp();
    auto delta = now - m_last_expiery;

    for (auto timer_index : m_active_timers)
    {
        auto& timer = m_timers[timer_index];

        timer.timeout -= std::min(delta, timer.timeout);
    }

    m_last_expiery = now;
}

std::optional<milliseconds>
TimerManager::Expire()
{
    m_in_expire = true;

    auto now = os::GetTimeStamp();
    auto delta = now - m_last_expiery;

    auto next_wakeup = kForever;

    // Make sure timers are sorted by expiery time
    SortActiveTimers();

    for (auto timer_index : m_active_timers)
    {
        auto& timer = m_timers[timer_index];

        if (!timer.cookie)
        {
            // Deleted or expired timer, ignore until deleted
            continue;
        }

        timer.timeout -= std::min(delta, timer.timeout);
        if (timer.timeout == 0ms)
        {
            auto next = timer.on_timeout();

            // Wake up the task if something expires
            m_semaphore.release();

            if (next)
            {
                // Periodic timer: Set the next timeout
                timer.timeout = *next;
            }
            else if (timer.cookie)
            {
                // Expired, so detach unless it was released by the callback
                timer.cookie->Detach();
            }
        }

        // Update the wakeup time if this is a valid (non-expired, non-deleted) timer
        if (timer.cookie && timer.timeout < next_wakeup)
        {
            next_wakeup = timer.timeout;
        }
    }

    next_wakeup = ActivatePendingTimers(next_wakeup);

    RemoveDeletedTimers();

    m_last_expiery = now;
    m_in_expire = false;

    if (next_wakeup == kForever)
    {
        return std::nullopt;
    }

    return next_wakeup;
}
