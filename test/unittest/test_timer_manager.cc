#include "test.hh"
#include "timer_manager.hh"

using namespace os;

namespace
{

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

std::weak_ptr<MockTime> g_mock_time;

class Fixture
{
public:
    Fixture()
    {
        g_mock_time = m_time;
    }

    ~Fixture()
    {
        g_mock_time.reset();
    }

    void AdvanceTime(milliseconds time)
    {
        m_time->AdvanceTime(time);
    }

    void SetTime(milliseconds time)
    {
        m_time->SetTime(time);
    }

    os::binary_semaphore m_sem {0};

private:
    std::shared_ptr<MockTime> m_time = std::make_shared<MockTime>();
};

class MockCallback
{
public:
    MAKE_MOCK0(OnTimeout, void());
};

} // namespace


milliseconds
os::GetTimeStamp()
{
    auto p = g_mock_time.lock();

    assert(p);
    return p->Now();
}


TEST_CASE_FIXTURE(Fixture, "the timer manager is empty by default")
{
    TimerManager manager(m_sem);

    REQUIRE(manager.Expire() == std::nullopt);
}

TEST_CASE_FIXTURE(Fixture, "a single shot timer is created")
{
    TimerManager manager(m_sem);
    MockCallback cb;

    auto timer = manager.StartTimer(1s, [&cb]() {
        cb.OnTimeout();
        return std::nullopt;
    });
    REQUIRE(timer);

    THEN("the next expiery is at the timer timeout")
    {
        REQUIRE(manager.Expire() == 1s);
    }

    WHEN("the timer is released")
    {
        timer = nullptr;

        AdvanceTime(1s);
        THEN("no more expiery")
        {
            REQUIRE(manager.Expire() == std::nullopt);
        }
    }

    WHEN("time advances")
    {
        auto r_cb = NAMED_REQUIRE_CALL(cb, OnTimeout());

        AdvanceTime(1s);
        auto expire_time = manager.Expire();
        THEN("the timer is invoked")
        {
            REQUIRE(r_cb);
        }
        AND_THEN("the timer is expired")
        {
            REQUIRE(timer->IsExpired());
        }

        AND_THEN("since this is a single-shot timer, no more expirey")
        {
            REQUIRE(expire_time == std::nullopt);
        }
    }
}

TEST_CASE_FIXTURE(Fixture, "the timer handler handles 32-bit wraps")
{
    TimerManager manager(m_sem);
    MockCallback cb;

    SetTime(0xFFFFFFFFms);
    manager.Expire();

    auto timer = manager.StartTimer(2ms, [&cb]() {
        cb.OnTimeout();
        return std::nullopt;
    });
    REQUIRE(timer);
    auto expire_time = manager.Expire();

    THEN("the next expiery is at the timer timeout")
    {
        REQUIRE(expire_time == 2ms);
        REQUIRE(timer->TimeLeft() == 2ms);
    }
    AND_WHEN("the time advances")
    {
        AdvanceTime(1ms);
        REQUIRE(manager.Expire() == 1ms);
        REQUIRE(timer->TimeLeft() == 1ms);

        auto r_cb = NAMED_REQUIRE_CALL(cb, OnTimeout());
        AdvanceTime(1ms);
        expire_time = manager.Expire();

        THEN("expiery is as expected")
        {
            r_cb = nullptr;
            REQUIRE(expire_time == std::nullopt);
            REQUIRE(timer->TimeLeft() == 0ms);
        }
    }
}


TEST_CASE_FIXTURE(Fixture, "a single shot timer is recreated after timeout")
{
    TimerManager manager(m_sem);
    MockCallback cb;

    auto timer = manager.StartTimer(1s, [&cb]() {
        cb.OnTimeout();
        return std::nullopt;
    });
    REQUIRE(timer);

    WHEN("time advances")
    {
        auto r_cb = NAMED_REQUIRE_CALL(cb, OnTimeout());

        AdvanceTime(1s);
        auto expire_time = manager.Expire();
        THEN("the timer is invoked")
        {
            REQUIRE(r_cb);
            REQUIRE(timer->IsExpired());
        }
        timer = nullptr;

        AND_WHEN("another timer is created afterwards")
        {
            AdvanceTime(10s);

            timer = manager.StartTimer(200ms, [&cb]() {
                cb.OnTimeout();
                return std::nullopt;
            });
            REQUIRE(manager.Expire() == 200ms);
            REQUIRE(timer);

            THEN("it also expires after a while")
            {
                AdvanceTime(100ms);

                REQUIRE(manager.Expire() == 100ms);
                REQUIRE_CALL(cb, OnTimeout());
                AdvanceTime(100ms);
                REQUIRE(manager.Expire() == std::nullopt);
            }
        }
    }
}


TEST_CASE_FIXTURE(Fixture, "a periodic timer is created")
{
    TimerManager manager(m_sem);
    MockCallback cb;

    auto timer = manager.StartTimer(1s, [&cb]() {
        cb.OnTimeout();
        return 100ms;
    });
    REQUIRE(timer);

    AdvanceTime(800ms);
    REQUIRE(manager.Expire() == 200ms);
    REQUIRE(timer->TimeLeft() == 200ms);
    WHEN("the initial time has passed")
    {
        auto r_cb = NAMED_REQUIRE_CALL(cb, OnTimeout());

        AdvanceTime(200ms);
        auto expire_time = manager.Expire();

        THEN("the callback timer is invoked")
        {
            REQUIRE(r_cb);
        }
        AND_THEN("the next timer is the period")
        {
            REQUIRE(expire_time == 100ms);
            REQUIRE(timer->TimeLeft() == 100ms);
        }

        AND_THEN("the peroidic timer is invoked after the period")
        {
            r_cb = NAMED_REQUIRE_CALL(cb, OnTimeout());
            AdvanceTime(100ms);
            expire_time = manager.Expire();

            REQUIRE(r_cb);
            REQUIRE(expire_time == 100ms);

            AdvanceTime(50ms);
            expire_time = manager.Expire();
            REQUIRE(expire_time == 50ms);
            REQUIRE(timer->TimeLeft() == 50ms);
        }
    }
}


TEST_CASE_FIXTURE(Fixture, "multiple timers are used")
{
    TimerManager manager(m_sem);
    MockCallback cb0, cb1;

    auto timer0 = manager.StartTimer(1s, [&cb0]() {
        cb0.OnTimeout();
        return 1s;
    });
    REQUIRE(timer0);

    auto timer1 = manager.StartTimer(300ms, [&cb1]() {
        cb1.OnTimeout();
        return std::nullopt;
    });
    REQUIRE(timer1);

    THEN("the first timer that expires is used for the timeout")
    {
        REQUIRE(manager.Expire() == 300ms);
    }

    WHEN("the first timer expires")
    {
        auto r_cb = NAMED_REQUIRE_CALL(cb1, OnTimeout());

        AdvanceTime(300ms);
        auto expire_time = manager.Expire();

        THEN("the first timer is invoked")
        {
            REQUIRE(r_cb);
            REQUIRE(expire_time == 700ms);
        }

        AND_WHEN("the second timer expires")
        {
            r_cb = NAMED_REQUIRE_CALL(cb0, OnTimeout());

            AdvanceTime(700ms);
            expire_time = manager.Expire();

            THEN("the second timer is invoked")
            {
                REQUIRE(r_cb);
                REQUIRE(expire_time == 1s);
            }
        }
    }
}


TEST_CASE_FIXTURE(Fixture, "a timer is released from its own callback")
{
    TimerManager manager(m_sem);

    std::unique_ptr<ITimer> timer;

    timer = manager.StartTimer(1s, [&timer]() {
        REQUIRE(timer);
        timer = nullptr;
        return std::nullopt;
    });
    REQUIRE(timer);

    AdvanceTime(1s);
    auto expire_time = manager.Expire();
    REQUIRE(expire_time == std::nullopt);
}
