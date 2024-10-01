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
        kForwarding,
        kRequestRoute,
        kDemo,
        kValueCount,
    };

    std::optional<milliseconds> OnActivation() final;
    hal::RawGpsData WaitForData(os::binary_semaphore& semaphore) final;

    void RunDemo();

    const MapMetadata m_map_metadata;
    ApplicationState& m_application_state;
    RouteService& m_route_service;

    std::unique_ptr<IRouteListener> m_route_listener;

    std::unique_ptr<RouteIterator> m_route_iterator;
    std::optional<Point> m_next_position;
    Point m_position;
    Vector m_direction;


    State m_state {State::kForwarding};

    int m_angle {0};
    uint32_t m_speed {0};
    os::binary_semaphore m_has_data_semaphore {0};
};
