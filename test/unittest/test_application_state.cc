#include "application_state.hh"
#include "test.hh"

TEST_CASE("it's possible to commit to the application state")
{
    ApplicationState state;

    auto s = state.Checkout();
    auto ro = state.CheckoutReadonly();

    // Default values
    REQUIRE(s->demo_mode == false);
    REQUIRE(s->gps_connected == false);
    REQUIRE(s->bluetooth_connected == false);

    s->bluetooth_connected = true;
    REQUIRE(ro->bluetooth_connected == false);

    // Commits
    s = nullptr;
    REQUIRE(ro->bluetooth_connected == true);

    auto s2 = state.Checkout();
    REQUIRE(s2->bluetooth_connected == true);
}

TEST_CASE("a change to the application state wakes up listeners")
{
    ApplicationState state;

    os::binary_semaphore sem_a {0};
    os::binary_semaphore sem_b {0};

    auto l_a = state.AttachListener(sem_a);
    auto l_b = state.AttachListener(sem_b);

    REQUIRE(sem_a.try_acquire() == false);
    REQUIRE(sem_b.try_acquire() == false);

    auto s = state.Checkout();
    s->demo_mode = true;
    s = nullptr;

    REQUIRE(sem_a.try_acquire() == true);
    REQUIRE(sem_b.try_acquire() == true);
}

TEST_CASE("no change to the application state doesn't wake up listeners")
{
    ApplicationState state;

    os::binary_semaphore sem_a {0};

    auto l_a = state.AttachListener(sem_a);

    REQUIRE(sem_a.try_acquire() == false);

    auto s = state.Checkout();
    s->demo_mode = false;
    s = nullptr;

    REQUIRE(sem_a.try_acquire() == false);
}