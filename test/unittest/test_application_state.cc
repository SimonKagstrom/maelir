#include "application_state.hh"
#include "semaphore.hh"
#include "test.hh"

TEST_SUITE_BEGIN("application_state");

TEST_CASE("The maelir application state has default values")
{
    ApplicationState state;

    REQUIRE(state.Get<AS::demo_mode>() == false);
    REQUIRE(state.Get<AS::gps_status>() == GpsStatus::kSilent);
    REQUIRE(state.Get<AS::configuration>()->color_mode == ColorMode::kColor);
    REQUIRE(state.Get<AS::configuration>()->show_speedometer == true);
}

TEST_CASE("A change to the application state wakes up listeners")
{
    ApplicationState state;

    os::binary_semaphore sem_a {0};
    os::binary_semaphore sem_b {0};

    auto l_a = state.AttachListener<AS::demo_mode>(sem_a);
    auto l_b = state.AttachListener<AS::demo_mode, AS::gps_status>(sem_b);

    REQUIRE(sem_a.try_acquire() == false);
    REQUIRE(sem_b.try_acquire() == false);

    state.Set<AS::demo_mode>(true);

    REQUIRE(sem_a.try_acquire() == true);
    REQUIRE(sem_b.try_acquire() == true);

    WHEN("the same value is written again")
    {
        state.Set<AS::demo_mode>(true);

        THEN("listeners are not woken up")
        {
            REQUIRE(sem_a.try_acquire() == false);
            REQUIRE(sem_b.try_acquire() == false);
        }
    }
}

TEST_CASE("Two snapshots can modify separate parts of the configuration")
{
    ApplicationState state;

    {
        auto s0 = state.CheckoutPartialSnapshot<AS::configuration>();
        auto s1 = state.CheckoutPartialSnapshot<AS::minute_average_speed>();

        s0.GetWritableReference<AS::configuration>().show_speedometer = false;
        s1.Set<AS::minute_average_speed>(12);
    }

    REQUIRE(state.Get<AS::configuration>()->show_speedometer == false);
    REQUIRE(state.Get<AS::minute_average_speed>() == 12);
}

TEST_SUITE_END();
