#pragma once

#include "semaphore.hh"
#include "time.hh"
#include "timer_manager.hh"

#include <atomic>
#include <optional>

namespace os
{

enum ThreadPriority : uint8_t
{
    kLow = 1,
    kNormal,
    kHigh,
};

class BaseThread
{
public:
    BaseThread();

    virtual ~BaseThread();

    void Awake()
    {
        m_semaphore.release();
    }

    void Start(uint8_t core = 0,
               ThreadPriority priority = ThreadPriority::kLow,
               uint32_t stack_size = 5000);

    void Stop()
    {
        m_running = false;
        Awake();
    }

protected:
    // The thread has just started
    virtual void OnStartup()
    {
    }

    /// @brief the thread has been awoken
    virtual std::optional<milliseconds> OnActivation() = 0;

    std::unique_ptr<ITimer> StartTimer(
        milliseconds timeout, std::function<std::optional<milliseconds>()> on_timeout = []() {
            return std::optional<milliseconds>();
        })
    {
        return m_timer_manager.StartTimer(timeout, on_timeout);
    }

    os::binary_semaphore& GetSemaphore()
    {
        return m_semaphore;
    }

private:
    struct Impl;

    std::optional<milliseconds> SelectWakeup(auto a, auto b) const
    {
        if (a && b)
        {
            return std::min(*a, *b);
        }
        else if (a)
        {
            return *a;
        }
        else if (b)
        {
            return *b;
        }

        // Unreachable
        return std::nullopt;
    }

    void ThreadLoop()
    {
        OnStartup();

        while (m_running)
        {
            auto thread_wakeup = OnActivation();
            auto timer_expiery = m_timer_manager.Expire();

            if (auto time = SelectWakeup(thread_wakeup, timer_expiery); time)
            {
                m_semaphore.try_acquire_for(*time);
            }
            else
            {
                m_semaphore.acquire();
            }
        }
    }

    std::atomic_bool m_running {true};
    binary_semaphore m_semaphore {0};
    Impl* m_impl {nullptr}; // Raw pointer to allow forward declaration
    TimerManager m_timer_manager;
};

} // namespace os
