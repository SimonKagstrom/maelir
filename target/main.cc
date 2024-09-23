#include "gps_reader.hh"
#include "gps_simulator.hh"
#include "route_service.hh"
#include "sdkconfig.h"
#include "target_display.hh"
#include "tile_producer.hh"
#include "tile_utils.hh"
#include "uart_gps.hh"
#include "ui.hh"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern "C" void
app_main(void)
{
    auto display = std::make_unique<DisplayTarget>();
    //auto gps = std::make_unique<UartGps>(UART_NUM_1,
    //                                     17,  // RX -> A0
    //                                     16); // TX -> A1
    auto gps = std::make_unique<GpsSimulator>();

    auto gps_reader = std::make_unique<GpsReader>(*gps);
    auto producer = std::make_unique<TileProducer>();
    auto route_service = std::make_unique<RouteService>();
    auto ui = std::make_unique<UserInterface>(
        *producer, *display, gps_reader->AttachListener(), route_service->AttachListener());

    gps->Start(0);
    gps_reader->Start(0);
    producer->Start(1);
    route_service->Start(0);
    ui->Start(1);

    route_service->RequestRoute({678, 865}, {1844, 777});

    while (true)
    {
        vTaskSuspend(nullptr);
    }
}
