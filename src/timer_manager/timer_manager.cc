#include "timer_manager.hh"

using namespace os;


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
        m_manager.m_pending_removals.push_back(m_entry_index);
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
    for (auto i = 0u; i < kMaxTimers; ++i)
    {
        m_timers[i].cookie = nullptr;
        m_free_timers.push_back(i);
    }
}

std::unique_ptr<ITimer>
TimerManager::StartTimer(milliseconds timeout,
                         std::function<std::optional<milliseconds>()> on_timeout)
{
    // Run old timers before (also to update the last expiery)
    Expire();

    if (m_free_timers.empty())
    {
        return nullptr;
    }

    auto index = m_free_timers.back();
    m_free_timers.pop_back();
    m_active_timers.push_back(index);

    auto cookie = new TimerImpl(*this, index);

    auto& timer = m_timers[index];

    timer.timeout = timeout;
    timer.on_timeout = std::move(on_timeout);
    timer.expired = false;
    timer.cookie = cookie;

    return std::unique_ptr<ITimer>(cookie);
}

std::optional<milliseconds>
TimerManager::Expire()
{
    constexpr auto kForever = milliseconds::max();

    auto now = os::GetTimeStamp();
    auto delta = now - m_last_expiery;

    auto next_wakeup = kForever;

    for (auto& timer_index : m_active_timers)
    {
        auto& timer = m_timers[timer_index];

        if (!timer.cookie)
        {
            // Deleted: Remove this timer
            continue;
        }

        if (timer.expired)
        {
            continue;
        }

        if (delta >= timer.timeout)
        {
            auto next = timer.on_timeout();

            if (next)
            {
                timer.timeout = *next;
            }
            else
            {
                timer.expired = true;
                timer.timeout = 0ms;
            }
        }
        else
        {
            timer.timeout -= delta;
        }


        if (timer.expired == false && timer.timeout < next_wakeup)
        {
            next_wakeup = timer.timeout;
        }
    }

    for (auto index : m_pending_removals)
    {
        auto it = std::find(m_active_timers.begin(), m_active_timers.end(), index);

        if (it != m_active_timers.end())
        {
            m_active_timers.erase(it);
        }

        m_free_timers.push_back(index);
    }
    m_pending_removals.clear();

    m_last_expiery = now;

    if (next_wakeup == kForever)
    {
        return std::nullopt;
    }

    return next_wakeup;
}
