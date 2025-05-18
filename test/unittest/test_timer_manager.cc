#include "test.hh"
#include "timer_manager.hh"
#include "mock_time.hh"

using namespace os;

namespace
{

class MockCallback
{
public:
    MAKE_MOCK0(OnTimeout, void());
};

class Fixture : public TimeFixture
{
};

} // namespace


TEST_CASE_FIXTURE(Fixture, "the timer manager is empty by default")
{
    TimerManager manager(m_sem);

    REQUIRE(manager.Expire() == std::nullopt);
}

TEST_CASE_FIXTURE(Fixture, "the timer manager can be filled without adverse effects")
{
    TimerManager manager(m_sem);

    std::vector<TimerHandle> timers;

    for (auto i = 0; i < kMaxTimers; i++)
    {
        auto t = manager.StartTimer(milliseconds(i + 1), []() { return std::nullopt; });
        REQUIRE(t);

        timers.push_back(std::move(t));
    }
    REQUIRE(manager.Expire() == 1ms);

    auto next = manager.StartTimer(1s, []() { return std::nullopt; });
    REQUIRE(next == nullptr);
    REQUIRE(manager.Expire() == 1ms);

    // Remove one, and make sure it's garbage collected
    WHEN("a timer is removed")
    {
        timers.pop_back();
        manager.Expire();

        THEN("a new one can be added")
        {
            // Then a new one can be added
            next = manager.StartTimer(1s, []() { return std::nullopt; });
            REQUIRE(next);
        }
    }

    WHEN("the first timer expires")
    {
        AdvanceTime(1ms);
        manager.Expire();

        THEN("a new one can be added")
        {
            next = manager.StartTimer(1s, []() { return std::nullopt; });
            REQUIRE(next);
        }
    }
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
            REQUIRE_FALSE(m_sem.try_acquire());
        }
    }

    WHEN("time advances")
    {
        auto r_cb = NAMED_REQUIRE_CALL(cb, OnTimeout());

        AdvanceTime(1s);
        REQUIRE_FALSE(m_sem.try_acquire());
        auto expire_time = manager.Expire();
        THEN("the timer is invoked")
        {
            REQUIRE(r_cb);
        }
        AND_THEN("the timer is expired")
        {
            REQUIRE(timer->IsExpired());
        }
        AND_THEN("the task is woke")
        {
            REQUIRE(m_sem.try_acquire());
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
            REQUIRE(expire_time == std::nullopt);
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

        AND_THEN("the periodic timer is invoked after the period")
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

    TimerHandle timer;

    WHEN("a single-shot timer is released from it's callback")
    {
        timer = manager.StartTimer(1s, [&timer]() {
            REQUIRE(timer);
            timer = nullptr;
            return std::nullopt;
        });
        REQUIRE(timer);

        AdvanceTime(1s);
        auto expire_time = manager.Expire();
        THEN("nothing ill happens")
        {
            REQUIRE(expire_time == std::nullopt);
            REQUIRE_FALSE(timer);
            REQUIRE(manager.Expire() == std::nullopt);
        }
    }

    WHEN("a periodic timer is released from it's callback")
    {
        timer = manager.StartTimer(1s, [&timer]() {
            timer = nullptr;
            return 10ms;
        });
        REQUIRE(timer);

        AdvanceTime(1s);
        auto expire_time = manager.Expire();
        THEN("nothing ill happens")
        {
            REQUIRE(expire_time == std::nullopt);
            REQUIRE_FALSE(timer);
            // Now no timers should be active
            REQUIRE(manager.Expire() == std::nullopt);
        }
    }
}

TEST_CASE_FIXTURE(Fixture, "timers are expired in order")
{
    TimerManager manager(m_sem);

    unsigned t10_0_order = 0;
    unsigned t20_0_order = 0;

    unsigned t20_1_order = 0;
    unsigned t10_1_order = 0;
    unsigned t15_periodic_order = 0;
    unsigned t20_then_10_periodic_order = 0;

    unsigned order = 1;

    auto t10_0 = manager.StartTimer(10ms, [&order, &t10_0_order]() {
        t10_0_order = order++;
        return std::nullopt;
    });
    auto t20_0 = manager.StartTimer(20ms, [&order, &t20_0_order]() {
        t20_0_order = order++;
        return std::nullopt;
    });

    auto expire_time = manager.Expire();
    REQUIRE(expire_time == 10ms);

    auto t20_periodic = manager.StartTimer(20ms, [&order, &t20_then_10_periodic_order]() {
        t20_then_10_periodic_order = order++;

        // First 20ms, then 10ms
        return 10ms;
    });

    auto t15_periodic = manager.StartTimer(15ms, [&order, &t15_periodic_order]() {
        t15_periodic_order = order++;
        return 15ms;
    });

    auto t20_1 = manager.StartTimer(20ms, [&order, &t20_1_order]() {
        t20_1_order = order++;
        return std::nullopt;
    });
    auto t10_1 = manager.StartTimer(10ms, [&order, &t10_1_order]() {
        t10_1_order = order++;
        return std::nullopt;
    });

    WHEN("time advances past the expiery times")
    {
        // The periodic timer should expire once
        AdvanceTime(25ms);
        expire_time = manager.Expire();

        THEN("all timers expire")
        {
            REQUIRE(expire_time == 10ms);
        }

        AND_THEN("the timers are expired in order")
        {
            REQUIRE(t10_0_order < t20_0_order);
            REQUIRE(t10_0_order < t20_1_order);

            REQUIRE(t10_1_order < t20_0_order);
            REQUIRE(t10_1_order < t20_1_order);

            REQUIRE(t10_0_order < t15_periodic_order);
            REQUIRE(t10_1_order < t15_periodic_order);

            REQUIRE(t15_periodic_order < t20_0_order);
            REQUIRE(t15_periodic_order < t20_1_order);
        }

        AND_WHEN("only perodic timers remain")
        {
            auto t15_before = t15_periodic_order;
            auto t20_then_10_before = t20_then_10_periodic_order;

            AdvanceTime(15ms);
            expire_time = manager.Expire();

            THEN("they also expire in order")
            {
                REQUIRE(t15_periodic_order > t15_before);
                REQUIRE(t20_then_10_periodic_order > t20_then_10_before);

                REQUIRE(t20_then_10_periodic_order < t15_periodic_order);
            }
        }
    }
}


TEST_CASE_FIXTURE(Fixture, "a new timer can be started from the timer callback")
{
    TimerManager manager(m_sem);
    MockCallback cb0, cb1;

    TimerHandle timer1;

    auto timer0 = manager.StartTimer(30ms, [&cb0, &cb1, &timer1, &manager]() {
        cb0.OnTimeout();

        timer1 = manager.StartTimer(20ms, [&cb1]() {
            cb1.OnTimeout();
            return std::nullopt;
        });

        return 30ms;
    });


    WHEN("the first timer expires")
    {
        REQUIRE_CALL(cb0, OnTimeout());
        AdvanceTime(30ms);
        auto expire_time = manager.Expire();

        THEN("the second timer is started")
        {
            REQUIRE(timer1);
            REQUIRE(expire_time == 20ms);
        }

        AND_WHEN("the new timer expires")
        {
            auto r_cb = NAMED_REQUIRE_CALL(cb1, OnTimeout());
            AdvanceTime(20ms);
            expire_time = manager.Expire();

            THEN("the new timer is invoked")
            {
                REQUIRE(r_cb);
                REQUIRE(expire_time == 10ms);
            }
        }
    }
}
