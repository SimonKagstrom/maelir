#include "base_thread.hh"
#include "encoder_input.hh"
#include "event_serializer.hh"
#include "target_gpio.hh"
#include "target_uart.hh"
#include "uart_event_forwarder.hh"
#include "uart_gps.hh"


#include <etl/queue_spsc_atomic.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern "C" void
app_main(void)
{
    auto button_gpio = std::make_unique<TargetGpio>(GPIO_NUM_2);
    auto pin_a_gpio = std::make_unique<TargetGpio>(GPIO_NUM_0);
    auto pin_b_gpio = std::make_unique<TargetGpio>(GPIO_NUM_1);

    // TODO: Pass multiple into one debonucer
    auto button_debouncer = std::make_unique<ButtonDebouncer>(*button_gpio);
    auto pin_a_debouncer = std::make_unique<ButtonDebouncer>(*button_gpio);
    auto pin_b_debouncer = std::make_unique<ButtonDebouncer>(*button_gpio);

    auto rotary_encoder = std::make_unique<RotaryEncoder>(*pin_a_debouncer, *pin_b_debouncer);

    auto encoder_input = std::make_unique<EncoderInput>(*rotary_encoder,
                                                        *button_debouncer, // Button
                                                        GPIO_NUM_3);       // Switch up
    auto gps_uart = std::make_unique<TargetUart>(UART_NUM_1,
                                                 9600,
                                                 GPIO_NUM_10,  // RX
                                                 GPIO_NUM_11); // TX

    auto gps_device = std::make_unique<UartGps>(*gps_uart);
    auto lcd_uart = std::make_unique<TargetUart>(UART_NUM_0,
                                                 115200,
                                                 GPIO_NUM_23,  // RX
                                                 GPIO_NUM_24); // TX
    auto gps_listener = std::make_unique<GpsListener>(*gps_device);
    auto forwarder = std::make_unique<UartEventForwarder>(*lcd_uart, *encoder_input, *gps_listener);

    button_debouncer->Start(0, os::ThreadPriority::kHigh);
    pin_a_debouncer->Start(0, os::ThreadPriority::kHigh);
    pin_b_debouncer->Start(0, os::ThreadPriority::kHigh);
    gps_listener->Start(0, os::ThreadPriority::kNormal);
    forwarder->Start(0, os::ThreadPriority::kNormal);

    while (true)
    {
        vTaskSuspend(nullptr);
    }
}
