#include "base_thread.hh"
#include "button_debouncer.hh"
#include "encoder_input.hh"
#include "event_serializer.hh"
#include "target_gpio.hh"
#include "target_uart.hh"
#include "uart_event_forwarder.hh"
#include "uart_gps.hh"

#include <etl/queue_spsc_atomic.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

constexpr auto kPinButton = GPIO_NUM_7;
constexpr auto kPinA = GPIO_NUM_9;
constexpr auto kPinB = GPIO_NUM_8;
constexpr auto kPinSwitchUp = GPIO_NUM_10;

extern "C" void
app_main(void)
{
    gpio_config_t io_conf = {};

    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = 1 << kPinButton;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    ESP_ERROR_CHECK(gpio_config(&io_conf));


    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1 << kPinA) | (1 << kPinB);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;

    ESP_ERROR_CHECK(gpio_config(&io_conf));

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1 << kPinSwitchUp);
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // Install the GPIO interrupt service
    gpio_install_isr_service(0);

    auto button_debouncer = std::make_unique<ButtonDebouncer>();

    auto pin_a_gpio = std::make_unique<TargetGpio>(kPinA);
    auto pin_b_gpio = std::make_unique<TargetGpio>(kPinB);
    auto switch_up_gpio = std::make_unique<TargetGpio>(kPinSwitchUp);
    auto button_gpio = button_debouncer->AddButton(
        std::make_unique<TargetGpio>(kPinButton, TargetGpio::Polarity::kActiveLow));

    auto rotary_encoder = std::make_unique<RotaryEncoder>(*pin_a_gpio, *pin_b_gpio);

    auto encoder_input = std::make_unique<EncoderInput>(*rotary_encoder,
                                                        *button_gpio,     // Button
                                                        *switch_up_gpio); // Switch up
    auto gps_uart = std::make_unique<TargetUart>(UART_NUM_1,
                                                 9600,
                                                 GPIO_NUM_3,  // RX
                                                 GPIO_NUM_4); // TX

    auto gps_device = std::make_unique<UartGps>(*gps_uart);
    auto lcd_uart = std::make_unique<TargetUart>(UART_NUM_0,
                                                 115200,
                                                 GPIO_NUM_20,  // RX
                                                 GPIO_NUM_21); // TX
    auto gps_listener = std::make_unique<GpsListener>(*gps_device);
    auto forwarder = std::make_unique<UartEventForwarder>(*lcd_uart, *encoder_input, *gps_listener);

    button_debouncer->Start(0, os::ThreadPriority::kHigh);
    gps_listener->Start(0, os::ThreadPriority::kNormal);
    forwarder->Start(0, os::ThreadPriority::kNormal);

    while (true)
    {
        vTaskSuspend(nullptr);
    }
}
