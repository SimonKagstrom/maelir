#include "event_serializer.hh"

using namespace serializer;

constexpr uint8_t kStartByte = 0xff;

void
Deserializer::PushData(std::span<const uint8_t> str)
{
    for (auto c : str)
    {
        m_input_queue.push(c);
    }
    RunStateMachine();
}

void
Deserializer::RunStateMachine()
{
    while (!m_input_queue.empty())
    {
        m_current_input = m_input_queue.front();
        m_input_queue.pop();

        switch (m_state)
        {
        case State::kWaitForStart:
            m_checksum = kStartByte;
            if (m_current_input == kStartByte)
            {
                EnterState(State::kWaitForLength);
            }
            break;

        case State::kWaitForLength:
            m_checksum ^= m_current_input;
            // Todo: Error handling
            m_message_length = m_current_input;
            EnterState(State::kReceiveData);
            break;

        case State::kReceiveData:
            if (m_message_buffer.size() < m_message_length)
            {
                m_checksum ^= m_current_input;
                m_message_buffer.push_back(m_current_input);
            }

            if (m_message_buffer.size() == m_message_length)
            {
                EnterState(State::kChecksum);
            }
            break;

        case State::kChecksum:
            if (m_current_input == m_checksum)
            {
                HandleEntry(m_message_buffer);
            }

            EnterState(State::kWaitForStart);
            break;

        case State::kValueCount:
            break;
        }
    }
}

void
Deserializer::EnterState(State state)
{
    m_state = state;

    switch (state)
    {
    case State::kWaitForStart:
        break;

    case State::kWaitForLength:
        m_message_length = 0;
        break;

    case State::kReceiveData:
        m_message_buffer.clear();
        break;

    case State::kChecksum:
        break;

    case State::kValueCount:
        // Handle value count
        break;
    }
}

void
Deserializer::HandleEntry(std::span<const uint8_t> data)
{
    if (data.size() == 2)
    {
        // Input + state
        auto event_limit = std::to_underlying(hal::IInput::EventType::kValueCount);

        // Check limits
        if (data[0] < event_limit && data[1] < 16)
        {
            m_input_events.push({hal::IInput::EventType {data[0]}, hal::IInput::State {data[1]}});
        }
    }

    if (data.size() == 1 + 4 * sizeof(float))
    {
        // validity, lat, lon, heading, speed
        hal::RawGpsData gps_data;

        auto valid = data[0];
        auto pos_valid = valid & 1;
        auto heading_valid = valid & 2;
        auto speed_valid = valid & 4;

        if (valid & 7)
        {
            if (pos_valid)
            {
                GpsPosition p;

                memcpy(&p.latitude, &data[1], sizeof(float));
                memcpy(&p.longitude, &data[1 + sizeof(float)], sizeof(float));
                gps_data.position = p;
            }
            if (heading_valid)
            {
                float heading;
                memcpy(&heading, &data[1 + 2 * sizeof(float)], sizeof(float));
                gps_data.heading = heading;
            }
            if (speed_valid)
            {
                float speed;
                memcpy(&speed, &data[1 + 3 * sizeof(float)], sizeof(float));
                gps_data.speed = speed;
            }

            m_gps_events.push(gps_data);
        }
    }
}

std::variant<std::monostate, hal::RawGpsData, InputEventState>
Deserializer::Deserialize()
{
    if (m_input_events.size() > 0)
    {
        InputEventState event = m_input_events.front();
        m_input_events.pop();
        return event;
    }
    if (m_gps_events.size() > 0)
    {
        hal::RawGpsData event = m_gps_events.front();
        m_gps_events.pop();
        return event;
    }

    return {};
}


namespace
{

void
DoSerialize(const serializer::InputEventState& data, etl::vector<uint8_t, 32>& str)
{
    str.push_back(2); // size
    auto raw_state = data.state.Raw();

    str.push_back(std::to_underlying(data.event));
    str.push_back(raw_state);
}

void
DoSerialize(const hal::RawGpsData& data, etl::vector<uint8_t, 32>& str)
{
    str.push_back(1 + 4 * sizeof(float));
    str.push_back(data.position.has_value() << 0 | data.heading.has_value() << 1 |
                  data.speed.has_value() << 2);

    str.resize(str.size() + 4 * sizeof(float));
    auto lat = &str[3];
    auto lon = &str[3 + sizeof(float)];
    auto heading = &str[3 + 2 * sizeof(float)];
    auto speed = &str[3 + 3 * sizeof(float)];

    if (data.position.has_value())
    {
        memcpy(lat, &data.position->latitude, sizeof(float));
        memcpy(lon, &data.position->longitude, sizeof(float));
    }
    if (data.heading.has_value())
    {
        memcpy(heading, &data.heading.value(), sizeof(float));
    }
    if (data.speed.has_value())
    {
        memcpy(speed, &data.speed.value(), sizeof(float));
    }
}

} // namespace

namespace serializer
{

template <typename T>
etl::vector<uint8_t, 32>
Serialize(const T& data)
{
    etl::vector<uint8_t, 32> str;
    str.push_back(kStartByte);

    DoSerialize(data, str);

    uint8_t checksum = 0x0;
    for (auto c : str)
    {
        checksum ^= c;
    }
    str.push_back(checksum);

    return str;
}

template etl::vector<uint8_t, 32> Serialize(const hal::RawGpsData& data);
template etl::vector<uint8_t, 32> Serialize(const InputEventState& data);

} // namespace serializer
