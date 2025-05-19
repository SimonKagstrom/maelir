#include "mock/mock_gps_port.hh"
#include "mock/mock_route_listener.hh"
#include "route_test_utils.hh"
#include "test.hh"
#include "thread_fixture.hh"
#include "trip_computer.hh"

using namespace route_test;

namespace
{

constexpr auto kMetersPerPixel = 2800.0f / 256;

class Fixture : public ThreadFixture
{
private:
    std::unique_ptr<MockGpsPort> m_gps_port;
    std::unique_ptr<MockRouteListener> m_route_listener;

public:
    Fixture()
        : m_gps_port(std::make_unique<MockGpsPort>())
        , m_route_listener(std::make_unique<MockRouteListener>())
        , gps_port(m_gps_port.get())
        , route_listener(m_route_listener.get())
    {
        ALLOW_CALL(*gps_port, DoAwakeOn(_));
        ALLOW_CALL(*route_listener, AwakeOn(_));

        trip_computer = std::make_unique<TripComputer>(m_application_state,
                                                       std::move(m_gps_port),
                                                       std::move(m_route_listener),
                                                       kMetersPerPixel,
                                                       32);
        SetThread(trip_computer.get());
    }

    ApplicationState m_application_state;
    MockGpsPort* gps_port;
    MockRouteListener* route_listener;
    std::unique_ptr<TripComputer> trip_computer;
};

} // namespace

TEST_CASE_FIXTURE(Fixture, "at startup, the trip computer average speed is 0")
{
    auto state = m_application_state.CheckoutReadonly();

    ALLOW_CALL(*gps_port, Poll()).RETURN(std::nullopt);
    ALLOW_CALL(*route_listener, Poll()).LR_RETURN(std::nullopt);

    DoRunLoop();
    REQUIRE(state->minute_average_speed == 0);
    REQUIRE(state->five_minute_average_speed == 0);
}

TEST_CASE_FIXTURE(Fixture, "the trip computer average speed is updated")
{
    auto state = m_application_state.CheckoutReadonly();

    auto gps_data = GpsData {.speed = 1.0f};

    ALLOW_CALL(*gps_port, Poll()).LR_RETURN(gps_data);
    ALLOW_CALL(*route_listener, Poll()).LR_RETURN(std::nullopt);

    DoRunLoop();
    REQUIRE(state->minute_average_speed == 1);
    REQUIRE(state->five_minute_average_speed == 0);

    gps_data.speed = 4.0f;
    // Push a few 4 knot entries
    for (auto i = 0u; i < 10; i++)
    {
        DoRunLoop();
    }
    REQUIRE(state->minute_average_speed == (1 + 10 * 4) / 11);
    REQUIRE(state->five_minute_average_speed == 0);

    WHEN("the history has filled up for one minute")
    {
        for (auto i = 0; i < 60 - 11; i++)
        {
            DoRunLoop();
        }

        THEN("it's transferred to the five minute history")
        {
            REQUIRE(state->minute_average_speed == 3);
            REQUIRE(state->five_minute_average_speed == 3);
        }

        AND_THEN("after a few minutes, the five minute history is updated accordingly")
        {
            gps_data.speed = 8.0f;

            for (auto i = 0; i < 4 * 60; i++)
            {
                DoRunLoop();
            }
            REQUIRE(state->five_minute_average_speed == (3 + 8 * 4) / 5);
        }
    }
}

TEST_CASE_FIXTURE(Fixture, "the trip computer updates distance to the target position")
{
    constexpr auto kRoute = std::array {ToIndex(0, 0), ToIndex(7, 0), ToIndex(15, 8)};
    constexpr uint32_t kRouteLength =
        (kPathFinderTileSize * 7 + kPathFinderTileSize * 8) * kMetersPerPixel;

    auto gps_data = GpsData {.speed = 20.0f};

    ALLOW_CALL(*gps_port, Poll()).LR_RETURN(gps_data);

    auto state = m_application_state.CheckoutReadonly();

    REQUIRE(state->route_passed_meters == 0);
    REQUIRE(state->route_total_meters == 0);

    WHEN("a new route is presented")
    {
        const auto kRouteEv = IRouteListener::Event {IRouteListener::EventType::kReady, kRoute};

        REQUIRE_CALL(*route_listener, Poll()).LR_RETURN(kRouteEv);
        DoRunLoop();

        THEN("the distance to the target is updated")
        {
            REQUIRE(state->route_total_meters == kRouteLength);
        }

        AND_WHEN("the boat moves along the route")
        {
            THEN("the passed distance is updated")
            {
            }
        }

        AND_WHEN("the route is dropped")
        {
            const auto kRouteDroppedEv =
                IRouteListener::Event {IRouteListener::EventType::kReleased, {}};

            REQUIRE_CALL(*route_listener, Poll()).LR_RETURN(kRouteDroppedEv);
            DoRunLoop();

            THEN("the distance to the target is zeroed")
            {
                REQUIRE(state->route_total_meters == 0);
            }
        }
        AND_WHEN("a new route is calculated")
        {
            const auto kRouteDroppedEv =
                IRouteListener::Event {IRouteListener::EventType::kCalculating, {}};

            REQUIRE_CALL(*route_listener, Poll()).LR_RETURN(kRouteDroppedEv);
            DoRunLoop();

            THEN("the distance to the target is zeroed")
            {
                REQUIRE(state->route_total_meters == 0);
            }
        }
    }
}
