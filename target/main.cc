#include "gps_reader.hh"
#include "gps_simulator.hh"
#include "sdkconfig.h"
#include "target_display.hh"
#include "tile_producer.hh"
#include "tile_utils.hh"
#include "ui.hh"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern "C" void
app_main(void)
{
    printf("Phnom world!\n");

    auto display = std::make_unique<DisplayTarget>();
    auto gps = std::make_unique<GpsSimulator>();

    auto gps_reader = std::make_unique<GpsReader>(*gps);
    auto producer = std::make_unique<TileProducer>(gps_reader->AttachListener());
    auto ui = std::make_unique<UserInterface>(*producer, *display, gps_reader->AttachListener());

    gps->Start();
    gps_reader->Start();
    producer->Start();
    ui->Start();

    while (true)
    {
        vTaskSuspend(nullptr);
    }
}
