#include "job_pool_thread.hh"
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

        auto owner = std::make_unique<TripComputer>(m_application_state,
                                                    std::move(m_gps_port),
                                                    std::move(m_route_listener),
                                                    kMetersPerPixel,
                                                    32);
        trip_computer = owner.get();

        // Pooled, like on the target. The job pool owns it from here on
        job_pool.AttachPooledThread(std::move(owner));
        SetThread(&job_pool);

        job_pool.Start("job_pool");

        // Attached threads are run on the first activation, so do that before the tests
        ALLOW_CALL(*route_listener, Poll()).RETURN(std::nullopt);
        job_pool.Awake();
        DoRunLoop();
    }

    // The trip computer uses the GPS data from the application state
    void SetGpsData(float speed, Point pixel_position = {})
    {
        auto qw = m_application_state
                      .CheckoutQueuedWriter<AS::position, AS::pixel_position, AS::gps_status>();

        qw.Set<AS::position>(GpsData {.speed = speed});
        qw.Set<AS::pixel_position>(pixel_position);
        qw.Set<AS::gps_status>(GpsStatus::kPositionValid);
    }

    void Step()
    {
        trip_computer->Awake();
        DoRunLoop();
    }

    ApplicationState m_application_state;
    MockGpsPort* gps_port;
    MockRouteListener* route_listener;
    TripComputer* trip_computer {nullptr};

    // After the state, since it owns the trip computer which refers to it
    JobPoolThread job_pool;
};

} // namespace

TEST_CASE_FIXTURE(Fixture, "at startup, the trip computer average speed is 0")
{
    ALLOW_CALL(*route_listener, Poll()).LR_RETURN(std::nullopt);

    Step();
    REQUIRE(m_application_state.Get<AS::minute_average_speed>() == 0);
    REQUIRE(m_application_state.Get<AS::five_minute_average_speed>() == 0);
}

TEST_CASE_FIXTURE(Fixture, "the trip computer average speed is updated")
{
    ALLOW_CALL(*route_listener, Poll()).LR_RETURN(std::nullopt);

    SetGpsData(1.0f);
    Step();
    REQUIRE(m_application_state.Get<AS::minute_average_speed>() == 1);
    REQUIRE(m_application_state.Get<AS::five_minute_average_speed>() == 0);

    SetGpsData(4.0f);
    // Push a few 4 knot entries
    for (auto i = 0u; i < 10; i++)
    {
        Step();
    }
    REQUIRE(m_application_state.Get<AS::minute_average_speed>() == (1 + 10 * 4) / 11);
    REQUIRE(m_application_state.Get<AS::five_minute_average_speed>() == 0);

    WHEN("the history has filled up for one minute")
    {
        for (auto i = 0; i < 60 - 11; i++)
        {
            Step();
        }

        THEN("it's transferred to the five minute history")
        {
            REQUIRE(m_application_state.Get<AS::minute_average_speed>() == 3);
            REQUIRE(m_application_state.Get<AS::five_minute_average_speed>() == 3);
        }

        AND_THEN("after a few minutes, the five minute history is updated accordingly")
        {
            SetGpsData(8.0f);

            for (auto i = 0; i < 4 * 60; i++)
            {
                Step();
            }
            REQUIRE(m_application_state.Get<AS::five_minute_average_speed>() == (3 + 8 * 4) / 5);
        }
    }
}

TEST_CASE_FIXTURE(Fixture, "the trip computer updates distance to the target position")
{
    auto NarrowResolution = [](uint32_t meters) {
        return (meters / 50) * 50; // Round to the nearest 50 meters
    };

    constexpr auto kRoute = std::array {ToIndex(0, 0), ToIndex(7, 0), ToIndex(15, 8)};
    constexpr uint32_t kFirstLegLength = kPathFinderTileSize * 7 * kMetersPerPixel;
    constexpr uint32_t kSecondLegLength = (kPathFinderTileSize * 8) * kMetersPerPixel;
    constexpr uint32_t kRouteLength = NarrowResolution(kFirstLegLength + kSecondLegLength);

    SetGpsData(20.0f);

    ALLOW_CALL(*route_listener, Poll()).RETURN(std::nullopt);

    REQUIRE(m_application_state.Get<AS::route_passed_meters>() == 0);
    REQUIRE(m_application_state.Get<AS::route_total_meters>() == 0);

    WHEN("a new route is presented")
    {
        const auto kRouteEv = IRouteListener::Event {IRouteListener::EventType::kReady, kRoute};

        REQUIRE_CALL(*route_listener, Poll()).LR_RETURN(kRouteEv);
        Step();

        THEN("the distance to the target is updated")
        {
            REQUIRE(m_application_state.Get<AS::route_total_meters>() == kRouteLength);
        }

        AND_WHEN("the boat moves along the route")
        {
            SetGpsData(20.0f, ToPoint(7, 0));
            Step();

            THEN("the passed distance is updated")
            {
                REQUIRE(m_application_state.Get<AS::route_passed_meters>() ==
                        NarrowResolution(kFirstLegLength));
                REQUIRE(m_application_state.Get<AS::route_total_meters>() == kRouteLength);
            }

            AND_WHEN("the target is almost reached")
            {
                auto target = ToPoint(15, 8);
                target.x -= kPathFinderTileSize / 8;

                SetGpsData(20.0f, target);
                Step();

                THEN("the passed distance is updated")
                {
                    REQUIRE(m_application_state.Get<AS::route_passed_meters>() == kRouteLength);
                    REQUIRE(m_application_state.Get<AS::route_passed_meters>() ==
                            m_application_state.Get<AS::route_total_meters>());
                }
            }
        }

        AND_WHEN("the route is dropped")
        {
            const auto kRouteDroppedEv =
                IRouteListener::Event {IRouteListener::EventType::kReleased, {}};

            REQUIRE_CALL(*route_listener, Poll()).LR_RETURN(kRouteDroppedEv);
            Step();

            THEN("the distance to the target is zeroed")
            {
                REQUIRE(m_application_state.Get<AS::route_total_meters>() == 0);
            }
        }
        AND_WHEN("a new route is calculated")
        {
            const auto kRouteDroppedEv =
                IRouteListener::Event {IRouteListener::EventType::kCalculating, {}};

            REQUIRE_CALL(*route_listener, Poll()).LR_RETURN(kRouteDroppedEv);
            Step();

            THEN("the distance to the target is zeroed")
            {
                REQUIRE(m_application_state.Get<AS::route_total_meters>() == 0);
            }
        }
    }
}
