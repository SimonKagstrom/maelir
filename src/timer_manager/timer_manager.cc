#include "timer_manager.hh"

using namespace os;

constexpr auto kForever = milliseconds::max();


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
        m_manager.EntryAt(m_entry_index).cookie = nullptr;
        m_manager.m_pending_removals[m_entry_index] = true;
    }

private:
    bool IsExpired() const final
    {
        return m_manager.EntryAt(m_entry_index).expired;
    }

    milliseconds TimeLeft() const final
    {
        return m_manager.EntryAt(m_entry_index).timeout;
    }

    TimerManager& m_manager;
    const uint8_t m_entry_index;
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

std::unique_ptr<ITimer>
TimerManager::StartTimer(milliseconds timeout,
                         std::function<std::optional<milliseconds>()> on_timeout)
{
    if (m_free_timers.none())
    {
        return nullptr;
    }

    auto index = m_free_timers.find_first(true);
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
    timer.expired = false;
    timer.cookie = cookie;

    return std::unique_ptr<ITimer>(cookie);
}

milliseconds
TimerManager::ActivatePendingTimers()
{
    auto next_wakeup = kForever;
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
        auto it = std::find(m_active_timers.begin(), m_active_timers.end(), index);

        if (it != m_active_timers.end())
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

        if (!timer.cookie || timer.expired)
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

            timer.expired = !next;
            if (next)
            {
                // Periodic timer: Set the next timeout
                timer.timeout = *next;
            }
        }

        // Update the wakeup time if this is a valid (non-expired, non-deleted) timer
        if (timer.expired == false && timer.cookie && timer.timeout < next_wakeup)
        {
            next_wakeup = timer.timeout;
        }
    }

    auto pending_wakeup = ActivatePendingTimers();
    next_wakeup = std::min(next_wakeup, pending_wakeup);

    RemoveDeletedTimers();

    m_last_expiery = now;
    m_in_expire = false;

    if (next_wakeup == kForever)
    {
        return std::nullopt;
    }

    return next_wakeup;
}
