#pragma once

#include "semaphore.hh"
#include "time.hh"
#include "timer_manager.hh"

#include <atomic>
#include <optional>

namespace os
{

constexpr auto kDefaultStackSize = 2048;

enum class ThreadPriority : uint8_t
{
    kLow = 1,
    kNormal,
    kHigh,
};

enum class ThreadCore : uint8_t
{
    kCore0 = 0,
    kCore1,
    // Add more if needed
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

    /**
     * @brief Start the thread
     *
     * @param name the name of the thread
     * @param core the core to pin the thread to
     * @param priority the thread priority (strict)
     * @param stack_size the stack size of the thread
     */
    void Start(const char* name, ThreadCore core, ThreadPriority priority, uint32_t stack_size);

    // The rest are just helpers for overloaded common cases
    void Start(const char* name)
    {
        Start(name, ThreadCore::kCore0, ThreadPriority::kLow, kDefaultStackSize);
    }

    void Start(const char* name, ThreadCore core)
    {
        Start(name, core, ThreadPriority::kLow, kDefaultStackSize);
    }

    void Start(const char* name, ThreadPriority priority)
    {
        Start(name, ThreadCore::kCore0, priority, kDefaultStackSize);
    }

    void Start(const char* name, ThreadCore core, ThreadPriority priority)
    {
        Start(name, core, priority, kDefaultStackSize);
    }

    void Start(const char* name, ThreadCore core, uint32_t stack_size)
    {
        Start(name, core, ThreadPriority::kLow, stack_size);
    }

    void Start(const char* name, uint32_t stack_size)
    {
        Start(name, ThreadCore::kCore0, ThreadPriority::kLow, stack_size);
    }

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

    TimerHandle StartTimer(
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
