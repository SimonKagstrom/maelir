#include "base_thread.hh"
#include "encoder_input.hh"
#include "event_serializer.hh"
#include "gps_listener.hh"
#include "target_uart.hh"
#include "uart_gps.hh"

#include <etl/queue_spsc_atomic.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace
{

class EventForwarder : public os::BaseThread, public hal::IInput::IListener
{
public:
    EventForwarder(hal::IInput& input, GpsListener& gps_listener)
        : m_input(input)
    {
        m_input.AttachListener(this);

        gps_listener.AttachListener([this](hal::RawGpsData data) {
            m_gps_queue.push(data);
            Awake();
        });
    }

private:
    std::optional<milliseconds> OnActivation() final
    {
        hal::IInput::Event event;
        hal::RawGpsData gps_data;

        m_buffer.clear();

        while (m_input_queue.pop(event))
        {
            printf("Got ev %d\n", (int)event.type);
            auto d =
                serializer::Serialize(serializer::InputEventState {event.type, m_input.GetState()});

            std::ranges::copy(d, std::back_inserter(m_buffer));
        }

        while (m_gps_queue.pop(gps_data))
        {
            auto d = serializer::Serialize(gps_data);

            std::ranges::copy(d, std::back_inserter(m_buffer));

            if (gps_data.position)
                printf(
                    "Location %f,%f\n", gps_data.position->latitude, gps_data.position->longitude);
            if (gps_data.heading)
                printf("Heading %f\n", *gps_data.heading);
            if (gps_data.speed)
                printf("speed %f\n", *gps_data.speed);
        }

        if (!m_buffer.empty())
        {
            // Send the data
        }

        return std::nullopt;
    }

    void OnInput(const hal::IInput::Event& event) final
    {
        m_input_queue.push(event);
        Awake();
    }

    hal::IInput& m_input;
    etl::queue_spsc_atomic<hal::IInput::Event, 16> m_input_queue;
    etl::queue_spsc_atomic<hal::RawGpsData, 16> m_gps_queue;

    etl::vector<uint8_t, 256> m_buffer;
};

}; // namespace


extern "C" void
app_main(void)
{
    printf("Este una planta\n");
    auto encoder_input = std::make_unique<EncoderInput>(GPIO_NUM_0,  // Pin A
                                                        GPIO_NUM_1,  // Pin B
                                                        GPIO_NUM_2,  // Button
                                                        GPIO_NUM_3); // Switch up
    printf("La planta de chili\n");
    auto gps_uart = std::make_unique<TargetUart>(UART_NUM_1,
                                                 9600,
                                                 GPIO_NUM_10,  // RX
                                                 GPIO_NUM_11); // TX

    auto gps_device = std::make_unique<UartGps>(*gps_uart);
    printf("Yo soy una planta\n");
    //    auto gps_device2 = std::make_unique<UartGps>(UART_NUM_0,
    //                                                 GPIO_NUM_23,  // RX
    //                                                 GPIO_NUM_24); // TX
    //
    auto gps_listener = std::make_unique<GpsListener>(*gps_device);
    auto forwarder = std::make_unique<EventForwarder>(*encoder_input, *gps_listener);

    gps_listener->Start(0, os::ThreadPriority::kNormal);
    forwarder->Start(0, os::ThreadPriority::kNormal);

    while (true)
    {
        vTaskSuspend(nullptr);
    }
}