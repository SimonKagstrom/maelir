#pragma once

#include "hal/i_gps.hh"
#include "hal/i_input.hh"

#include <etl/queue.h>
#include <etl/vector.h>
#include <span>
#include <string_view>
#include <variant>

namespace serializer
{

struct InputEventState
{
    hal::IInput::EventType event;
    hal::IInput::State state;
};

template <typename T>
etl::vector<uint8_t, 32> Serialize(const T& data);

class Deserializer
{
public:
    enum class State : uint8_t
    {
        kWaitForStart,
        kWaitForLength,
        kReceiveData,
        kChecksum,

        kValueCount,
    };

    Deserializer() = default;

    void PushData(std::span<const uint8_t> str);

    std::variant<std::monostate, hal::RawGpsData, InputEventState> Deserialize();

    // For unit testing
protected:
    void HandleEntry(std::span<const uint8_t> data);

private:
    void RunStateMachine();

    void EnterState(State state);

    State m_state {State::kWaitForStart};

    etl::queue<uint8_t, 64> m_input_queue;
    etl::vector<uint8_t, 32> m_message_buffer;

    etl::queue<hal::RawGpsData, 8> m_gps_events;
    etl::queue<InputEventState, 8> m_input_events;

    uint8_t m_current_input {0};
    uint8_t m_message_length {0};
    uint8_t m_checksum {0};
};

} // namespace serializer
