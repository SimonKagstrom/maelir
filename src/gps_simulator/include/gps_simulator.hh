#pragma once

#include "application_state.hh"
#include "base_thread.hh"
#include "hal/i_gps.hh"
#include "route_service.hh"


class GpsSimulator : public hal::IGps, public os::BaseThread
{
public:
    GpsSimulator(const MapMetadata& metadata,
                 ApplicationState& application_state,
                 RouteService& route_service);

private:
    enum class State
    {
        kIdle,
        kRequestRoute,
        kWaitForRoute,
        kDemo,
        kExitDemo,
        kValueCount,
    };

    std::optional<milliseconds> OnActivation() final;
    std::optional<hal::RawGpsData> WaitForData(os::binary_semaphore& semaphore) final;

    void RunDemo();

    const MapMetadata m_map_metadata;
    ApplicationState& m_application_state;
    RouteService& m_route_service;

    std::unique_ptr<IRouteListener> m_route_listener;
    std::unique_ptr<ApplicationState::IListener> m_application_state_listener;

    std::unique_ptr<RouteIterator> m_route_iterator;
    bool m_route_pending {false};
    std::optional<Point> m_next_position;
    std::vector<IndexType> m_route;
    Point m_position;
    Vector m_direction;


    State m_state {State::kIdle};

    int m_angle {0};
    float m_target_speed {0};
    float m_speed {0};
    os::binary_semaphore m_has_data_semaphore {0};
};
