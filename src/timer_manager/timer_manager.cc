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
    if (m_free_timers.empty())
    {
        return nullptr;
    }

    auto index = m_free_timers.back();
    m_free_timers.pop_back();

    if (m_in_expire)
    {
        m_pending_additions.push_back(index);
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
    if (m_pending_additions.empty())
    {
        return next_wakeup;
    }

    for (auto index : m_pending_additions)
    {
        m_active_timers.push_back(index);
        const auto& timer = m_timers[index];

        next_wakeup = std::min(next_wakeup, timer.timeout);
    }
    SortActiveTimers();
    m_pending_additions.clear();

    return next_wakeup;
}

void
TimerManager::BumpTime()
{
    auto now = os::GetTimeStamp();
    auto delta = now - m_last_expiery;

    for (auto timer_index : m_active_timers)
    {
        auto& timer = m_timers[timer_index];

        if (delta >= timer.timeout)
        {
            timer.timeout = 0ms;
        }
        else
        {
            timer.timeout -= delta;
        }
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

    auto pending_wakeup = ActivatePendingTimers();
    next_wakeup = std::min(next_wakeup, pending_wakeup);

    m_last_expiery = now;
    m_in_expire = false;

    if (next_wakeup == kForever)
    {
        return std::nullopt;
    }

    return next_wakeup;
}
