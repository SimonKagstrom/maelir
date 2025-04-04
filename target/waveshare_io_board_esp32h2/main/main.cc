#include "base_thread.hh"
#include "encoder_input.hh"
#include "event_serializer.hh"
#include "target_uart.hh"
#include "uart_event_forwarder.hh"
#include "uart_gps.hh"

#include <etl/queue_spsc_atomic.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern "C" void
app_main(void)
{
    auto encoder_input = std::make_unique<EncoderInput>(GPIO_NUM_0,  // Pin A
                                                        GPIO_NUM_1,  // Pin B
                                                        GPIO_NUM_2,  // Button
                                                        GPIO_NUM_3); // Switch up
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

    gps_listener->Start(0, os::ThreadPriority::kNormal);
    forwarder->Start(0, os::ThreadPriority::kNormal);

    while (true)
    {
        vTaskSuspend(nullptr);
    }
}