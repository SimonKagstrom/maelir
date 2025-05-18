#pragma once

#include "application_state.hh"
#include "base_thread.hh"
#include "gps_port.hh"
#include "i_route_listener.hh"
#include "route_iterator.hh"

#include <etl/vector.h>

class TripComputer : public os::BaseThread
{
public:
    TripComputer(const MapMetadata& metadata,
                 ApplicationState& application_state,
                 std::unique_ptr<IGpsPort> gps_port,
                 std::unique_ptr<IRouteListener> route_listener);

private:
    template <size_t Size>
    class HistoryBuffer
    {
    public:
        /**
         * @brief Push a new value to the history buffer
         *
         * @param value the value to push
         * @return true if the buffer has wrapped around (index is 0 again)
         */
        bool Push(uint8_t value);

        /**
         * @brief Return the average of the values in the history buffer
         *
         * @return the average
         */
        uint8_t Average() const;

    private:
        etl::vector<uint8_t, Size> m_history;
        uint8_t m_index {0};
    };

    std::optional<milliseconds> OnActivation() final;

    void HandleSpeed(float speed_knots);

    ApplicationState& m_application_state;
    std::unique_ptr<IGpsPort> m_gps_port;
    std::unique_ptr<IRouteListener> m_route_listener;

    HistoryBuffer<60> m_second_history;
    HistoryBuffer<5> m_minute_history;

    os::TimerHandle m_gps_tick;
};
