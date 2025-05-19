#pragma once

#include "base_thread.hh"
#include "mock_time.hh"
#include "test.hh"

class ThreadFixture : public TimeFixture
{
public:
    void SetThread(os::BaseThread* thread)
    {
        m_thread = thread;
    }

    std::optional<milliseconds> DoRunLoop()
    {
        assert(m_thread);
        return m_thread->RunLoop();
    }

private:
    os::BaseThread* m_thread {nullptr};
};
