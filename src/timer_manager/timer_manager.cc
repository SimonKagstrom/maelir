#include "timer_manager.hh"

using namespace os;


class TimerManager::TimerImpl : public ITimer
{
public:
    TimerImpl(TimerManager* parent,
              milliseconds timeout,
              std::function<std::optional<milliseconds>()> on_timeout)
        : m_parent(parent)
        , m_timeout(timeout)
        , m_on_timeout(on_timeout)
    {
        m_parent->m_timers.push_back(this);
    }

    ~TimerImpl()
    {
        if (auto it = std::ranges::find(m_parent->m_timers, this); it != m_parent->m_timers.end())
        {
            m_parent->m_timers.erase(it);
        }
    }

    static std::optional<milliseconds> Update(TimerImpl* timer, milliseconds delta)
    {
        auto left = timer->m_timeout;

        if (delta >= left)
        {
            timer->m_expired = true;
            timer->m_timeout = 0ms;

            // Might not be valid after the callback, i.e., the timer pointer might be dead
            auto next = timer->m_on_timeout();
            timer->m_parent->m_semaphore.release();
            if (next)
            {
                // Reschedule
                timer->m_timeout = *next;
                return *next;
            }

            return std::nullopt;
        }
        else
        {
            left -= delta;
            timer->m_timeout = left;
        }

        return left;
    }

private:
    bool IsExpired() const final
    {
        return m_expired;
    }

    milliseconds TimeLeft() const
    {
        return m_timeout;
    }

    TimerManager* m_parent;
    milliseconds m_timeout;
    std::function<std::optional<milliseconds>()> m_on_timeout;
    bool m_expired {false};
};


TimerManager::TimerManager(os::binary_semaphore& semaphore)
    : m_semaphore(semaphore)
    , m_last_expiery(os::GetTimeStamp())
{
}

std::unique_ptr<ITimer>
TimerManager::StartTimer(milliseconds timeout,
                         std::function<std::optional<milliseconds>()> on_timeout)
{
    if (m_timers.full())
    {
        return nullptr;
    }
    // Run old timers before
    Expire();

    return std::unique_ptr<ITimer>(new TimerImpl(this, timeout, on_timeout));
}

std::optional<milliseconds>
TimerManager::Expire()
{
    etl::vector<TimerImpl*, kMaxTimers> expired;
    std::optional<milliseconds> next_wakeup;

    auto now = os::GetTimeStamp();
    auto delta = now - m_last_expiery;

    for (auto timer : m_timers)
    {
        auto next = TimerImpl::Update(timer, delta);
        if (next == std::nullopt)
        {
            expired.push_back(timer);
        }
        else
        {
            if (!next_wakeup || *next < *next_wakeup)
            {
                next_wakeup = next;
            }
        }
    }

    for (auto expired : expired)
    {
        m_timers.erase(std::ranges::find(m_timers, expired));
    }

    m_last_expiery = now;

    return next_wakeup;
}
