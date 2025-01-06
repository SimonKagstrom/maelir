#pragma once

#include "base_thread.hh"

#include <atomic>
#include <etl/mutex.h>
#include <etl/vector.h>

class ApplicationState
{
public:
    class IListener
    {
    public:
        virtual ~IListener() = default;
    };

    struct State
    {
        virtual ~State() = default;

        bool demo_mode {false};
        bool gps_connected {false};
        bool bluetooth_connected {false};
        bool show_speedometer {true};

        auto operator==(const State& other) const
        {
            return demo_mode == other.demo_mode && gps_connected == other.gps_connected &&
                   bluetooth_connected == other.bluetooth_connected &&
                   show_speedometer == other.show_speedometer;
        }
    };

    std::unique_ptr<IListener> AttachListener(os::binary_semaphore& semaphore);

    // Checkout a local copy of the global state. Rewritten when the unique ptr is released
    std::unique_ptr<State> Checkout();

    const State* CheckoutReadonly() const;

private:
    class ListenerImpl;
    class StateImpl;

    void Commit(const StateImpl* state);

    State m_global_state;
    etl::mutex m_mutex;
    etl::vector<ListenerImpl*, 4> m_listeners;
};
