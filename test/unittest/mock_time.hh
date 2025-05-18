#pragma once

#include "semaphore.hh"
#include "test.hh"
#include "time.hh"

class MockTime
{
public:
    void SetTime(milliseconds time)
    {
        m_time = time;
    }

    void AdvanceTime(milliseconds time)
    {
        m_time += time;
    }

    auto Now()
    {
        return m_time;
    }

private:
    milliseconds m_time {10s};
};

class TimeFixture
{
public:
    TimeFixture();

    ~TimeFixture();

    void AdvanceTime(milliseconds time);

    void SetTime(milliseconds time);

    os::binary_semaphore m_sem {0};

private:
    std::shared_ptr<MockTime> m_time = std::make_shared<MockTime>();
};
