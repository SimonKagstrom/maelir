#include "encoder_input.hh"
#include "uart_gps.hh"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>


extern "C" void
app_main(void)
{
    // Currently a hack is used to disable shared interrupts which are not compatible with edge.
    printf("Este una planta\n");
    auto encoder_input = std::make_unique<EncoderInput>(GPIO_NUM_0,  // Pin A
                                                        GPIO_NUM_1,  // Pin B
                                                        GPIO_NUM_2,  // Button
                                                        GPIO_NUM_3); // Switch up
    printf("La planta de chili\n");
    auto gps_device = std::make_unique<UartGps>(UART_NUM_1,
                                                GPIO_NUM_10,  // RX
                                                GPIO_NUM_11); // TX
    printf("Yo soy una planta\n");
//    auto gps_device2 = std::make_unique<UartGps>(UART_NUM_0,
//                                                 GPIO_NUM_23,  // RX
//                                                 GPIO_NUM_24); // TX
//
    gpio_config_t io_conf = {};

    constexpr auto kLed = GPIO_NUM_8;

    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 1 << kLed;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);


    gpio_set_level(kLed, 0);
    auto on = 1;
    while (true)
    {
        os::Sleep(1s);
        gpio_set_level(kLed, on);
        on = !on;
        printf("Chiliplanta\n");

        auto X = static_cast<hal::IGps*>(gps_device.get());
        os::binary_semaphore sem {0};
        auto x = X->WaitForData(sem);
        if (x)
        {
            printf("Got GPS data\n");
        }
    }
}