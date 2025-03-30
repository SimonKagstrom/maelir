#include "event_serializer.hh"
#include "test.hh"

using namespace serializer;

// https://stackoverflow.com/questions/208433/how-do-i-write-a-short-literal-in-c
inline std::uint8_t
operator""_u8(unsigned long long value)
{
    return static_cast<std::uint8_t>(value);
}

namespace
{

class IntrospectiveDeserializer : public serializer::Deserializer
{
public:
    void DoHandleEntry(std::span<const uint8_t> data)
    {
        HandleEntry(data);
    }
};

} // namespace

TEST_CASE("the deserializer handles entry data")
{
    IntrospectiveDeserializer d;

    THEN("too short is dropped")
    {
        d.DoHandleEntry(std::array {0x01_u8});
        auto v = d.Deserialize();

        REQUIRE_FALSE(std::holds_alternative<hal::RawGpsData>(v));
        REQUIRE_FALSE(std::holds_alternative<InputEventState>(v));
    }

    THEN("neither input nor gps data is dropped")
    {
        d.DoHandleEntry(std::array {0x01_u8, 0x02_u8, 0x03_u8});
        auto v = d.Deserialize();

        REQUIRE_FALSE(std::holds_alternative<hal::RawGpsData>(v));
        REQUIRE_FALSE(std::holds_alternative<InputEventState>(v));
    }

    WHEN("the input matches input events")
    {
        auto valid = std::array {std::to_underlying(hal::IInput::EventType::kRight),
                                 std::to_underlying(hal::IInput::StateType::kButtonDown)};
        auto invalid_1 = std::array {std::to_underlying(hal::IInput::EventType::kValueCount),
                                     std::to_underlying(hal::IInput::StateType::kButtonDown)};
        auto invalid_2 =
            std::array {std::to_underlying(hal::IInput::EventType::kValueCount), 17_u8};

        AND_WHEN("the event is invalid")
        {
            d.DoHandleEntry(invalid_1);
            auto v = d.Deserialize();

            THEN("it dropped")
            {
                REQUIRE_FALSE(std::holds_alternative<InputEventState>(v));
            }
        }
        AND_WHEN("the state is invalid")
        {
            d.DoHandleEntry(invalid_2);
            auto v = d.Deserialize();

            THEN("it dropped")
            {
                REQUIRE_FALSE(std::holds_alternative<InputEventState>(v));
            }
        }
        AND_WHEN("the input is valid")
        {
            d.DoHandleEntry(valid);
            auto v = d.Deserialize();

            THEN("it can be read")
            {
                REQUIRE_FALSE(std::holds_alternative<hal::RawGpsData>(v));
                REQUIRE(std::holds_alternative<InputEventState>(v));

                auto ev = std::get<InputEventState>(v).event;
                auto st = std::get<InputEventState>(v).state;

                REQUIRE(ev == hal::IInput::EventType::kRight);
                REQUIRE(st.IsActive(hal::IInput::StateType::kButtonDown));
            }
        }
    }

    WHEN("the input is matches gps events")
    {
        GpsPosition gps_pos {59.1f, 18.2f};
        float heading = 90.0f;
        float speed = 13.0f;

        std::array<uint8_t, 4 * sizeof(float) + 1> data {};
        memcpy(data.data() + 1, &gps_pos, sizeof(gps_pos));
        memcpy(data.data() + 1 + sizeof(gps_pos), &heading, sizeof(heading));
        memcpy(data.data() + 1 + sizeof(gps_pos) + sizeof(heading), &speed, sizeof(speed));

        WHEN("only the GPS position is valid")
        {
            data[0] = 0x01;
            d.DoHandleEntry(data);
            auto v = d.Deserialize();

            THEN("it's deserialized, but nothing else")
            {
                REQUIRE_FALSE(std::holds_alternative<InputEventState>(v));
                REQUIRE(std::holds_alternative<hal::RawGpsData>(v));

                auto gps_data = std::get<hal::RawGpsData>(v);
                REQUIRE(gps_data.position.has_value());
                REQUIRE(gps_data.heading == std::nullopt);
                REQUIRE(gps_data.speed == std::nullopt);
                REQUIRE(*gps_data.position == gps_pos);
            }
        }

        AND_THEN("heading can be deserialized")
        {
            data[0] = 0x02;
            d.DoHandleEntry(data);
            auto v = d.Deserialize();

            REQUIRE(std::holds_alternative<hal::RawGpsData>(v));

            auto gps_data = std::get<hal::RawGpsData>(v);
            REQUIRE(gps_data.position == std::nullopt);
            REQUIRE(gps_data.heading.has_value());
            REQUIRE(gps_data.speed == std::nullopt);
            REQUIRE(*gps_data.heading == heading);
        }

        AND_THEN("speed can be deserialized")
        {
            data[0] = 0x04;
            d.DoHandleEntry(data);
            auto v = d.Deserialize();

            REQUIRE(std::holds_alternative<hal::RawGpsData>(v));

            auto gps_data = std::get<hal::RawGpsData>(v);
            REQUIRE(gps_data.position == std::nullopt);
            REQUIRE(gps_data.heading == std::nullopt);
            REQUIRE(gps_data.speed.has_value());
            REQUIRE(*gps_data.speed == speed);
        }

        AND_WHEN("when nothing is valid")
        {
            d.DoHandleEntry(data);
            auto v = d.Deserialize();

            THEN("nothing is deserialized")
            {
                REQUIRE_FALSE(std::holds_alternative<hal::RawGpsData>(v));
                REQUIRE_FALSE(std::holds_alternative<InputEventState>(v));
            }
        }
    }
}

TEST_CASE("serialized data can be deserialized")
{
    IntrospectiveDeserializer d;

    WHEN("an input event is serialized")
    {
        auto input_event = InputEventState {
            .event = hal::IInput::EventType::kLeft,
            .state = hal::IInput::State(std::to_underlying(hal::IInput::StateType::kSwitchUp)),
        };
        auto data = Serialize(input_event);

        THEN("it can be deserialized")
        {

            d.PushData(data);
            auto v = d.Deserialize();

            REQUIRE(std::holds_alternative<InputEventState>(v));
            auto ev = std::get<InputEventState>(v);
            REQUIRE(ev.event == input_event.event);
            REQUIRE(ev.state.Raw() == input_event.state.Raw());

            AND_THEN("no more events are deserialized")
            {
                auto v2 = d.Deserialize();
                REQUIRE_FALSE(std::holds_alternative<InputEventState>(v2));
                REQUIRE_FALSE(std::holds_alternative<hal::RawGpsData>(v2));
            }
        }

        AND_WHEN("the checksum is incorrect")
        {
            data[data.size() - 1]++;

            d.PushData(data);
            auto v = d.Deserialize();

            THEN("it is dropped")
            {
                REQUIRE_FALSE(std::holds_alternative<InputEventState>(v));
            }
        }

        AND_WHEN("the length is incorrect")
        {
            data[1]++;
            d.PushData(data);
            auto v = d.Deserialize();

            THEN("it is dropped")
            {
                REQUIRE_FALSE(std::holds_alternative<InputEventState>(v));
            }
        }

        AND_WHEN("the start code is incorrect")
        {
            data[0]++;
            d.PushData(data);
            auto v = d.Deserialize();

            THEN("it is dropped")
            {
                REQUIRE_FALSE(std::holds_alternative<InputEventState>(v));
            }
        }

        AND_WHEN("additional data is added before and after")
        {
            d.PushData(std::array {0x01_u8, 0x02_u8});
            d.PushData(data);
            auto v = d.Deserialize();
            d.PushData(std::array {0x01_u8, 0x02_u8});

            THEN("the event is deserialized")
            {
                REQUIRE(std::holds_alternative<InputEventState>(v));
            }
        }
    }

    WHEN("an gps event is serialized")
    {
        auto gps_pos = GpsPosition {59.1f, 18.2f};
        auto heading = 90.0f;
        auto speed = 13.0f;

        hal::RawGpsData gps_data;
        gps_data.position = gps_pos;
        gps_data.heading = heading;
        gps_data.speed = speed;

        THEN("it can be deserialized")
        {
            auto data = Serialize(gps_data);

            d.PushData(data);
            auto v = d.Deserialize();

            REQUIRE(std::holds_alternative<hal::RawGpsData>(v));
            auto ev = std::get<hal::RawGpsData>(v);
            REQUIRE(ev.position.has_value());
            REQUIRE(ev.heading.has_value());
            REQUIRE(ev.speed.has_value());
            REQUIRE(*ev.position == gps_pos);
            REQUIRE(*ev.heading == heading);
            REQUIRE(*ev.speed == speed);
        }

        AND_THEN("partial events are handled")
        {
            gps_data.heading = std::nullopt;

            auto data = Serialize(gps_data);

            d.PushData(data);
            auto v = d.Deserialize();

            REQUIRE(std::holds_alternative<hal::RawGpsData>(v));
            auto ev = std::get<hal::RawGpsData>(v);
            REQUIRE(ev.position.has_value());
            REQUIRE_FALSE(ev.heading.has_value());
            REQUIRE(ev.speed.has_value());
        }
    }


    WHEN("a string of events are sent")
    {
        auto i0 = InputEventState {
            .event = hal::IInput::EventType::kLeft,
            .state = hal::IInput::State(std::to_underlying(hal::IInput::StateType::kSwitchUp)),
        };
        auto i1 = InputEventState {
            .event = hal::IInput::EventType::kSwitchDown,
            .state = hal::IInput::State(std::to_underlying(hal::IInput::StateType::kButtonDown)),
        };
        hal::RawGpsData g0 {.position = std::nullopt, .heading = std::nullopt, .speed = 13.0f};

        std::vector<uint8_t> all_data;
        auto gps_data = Serialize(g0);
        auto i0_data = Serialize(i0);
        auto i1_data = Serialize(i1);

        std::ranges::copy(i0_data, std::back_inserter(all_data));
        std::ranges::copy(gps_data, std::back_inserter(all_data));
        std::ranges::copy(i1_data, std::back_inserter(all_data));

        THEN("all are handled, with input events prioritized")
        {
            d.PushData(all_data);
            auto v0 = d.Deserialize();
            auto v1 = d.Deserialize();
            auto v2 = d.Deserialize();

            REQUIRE(std::holds_alternative<InputEventState>(v0));
            REQUIRE(std::holds_alternative<InputEventState>(v1));
            REQUIRE(std::holds_alternative<hal::RawGpsData>(v2));

            REQUIRE(std::get<InputEventState>(v0).event == i0.event);
            REQUIRE(std::get<InputEventState>(v0).state.Raw() == i0.state.Raw());
            REQUIRE(std::get<InputEventState>(v1).event == i1.event);
            REQUIRE(std::get<InputEventState>(v1).state.Raw() == i1.state.Raw());
            REQUIRE(std::get<hal::RawGpsData>(v2).position == std::nullopt);
            REQUIRE(std::get<hal::RawGpsData>(v2).heading == std::nullopt);
            REQUIRE(std::get<hal::RawGpsData>(v2).speed.has_value());
        }
    }
}
