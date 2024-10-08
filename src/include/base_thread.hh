#pragma once

#include "semaphore.hh"
#include "time.hh"

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

    void Start(uint8_t core = 0, ThreadPriority priority = ThreadPriority::kLow);

    void Stop()
    {
        m_running = false;
        Awake();
    }

protected:
    /// @brief the thread has been awoken
    virtual std::optional<milliseconds> OnActivation() = 0;

    os::binary_semaphore& GetSemaphore()
    {
        return m_semaphore;
    }

private:
    struct Impl;

    void ThreadLoop()
    {
        while (m_running)
        {
            auto time = OnActivation();
            if (time)
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
};

} // namespace os
