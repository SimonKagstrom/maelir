#include "encoder_input.hh"
#include "gps_mux.hh"
#include "gps_reader.hh"
#include "gps_simulator.hh"
#include "route_service.hh"
#include "sdkconfig.h"
#include "target_nvm.hh"
#include "storage.hh"
#include "target_display.hh"
#include "tile_producer.hh"
#include "uart_gps.hh"
#include "ui.hh"

#include <esp_partition.h>
#include <esp_random.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern "C" void
app_main(void)
{
    auto map_partition =
        esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, "map_data");
    assert(map_partition);

    const void* p = nullptr;
    esp_partition_mmap_handle_t handle = 0;
    if (esp_partition_mmap(esp_partition_get(map_partition),
                           0,
                           esp_partition_get(map_partition)->size,
                           ESP_PARTITION_MMAP_DATA,
                           &p,
                           &handle) != ESP_OK)
    {
        assert(false);
    }
    esp_partition_iterator_release(map_partition);
    srand(esp_random());

    auto map_metadata = reinterpret_cast<const MapMetadata*>(p);

    auto target_nvm = std::make_unique<NvmTarget>();

    ApplicationState state;

    //state.Checkout()->demo_mode = false;
    state.Checkout()->demo_mode = true;

    auto encoder_input = std::make_unique<EncoderInput>(6,  // Pin A -> 6 (MISO)
                                                        7,  // Pin B -> 7 (MOSI)
                                                        5); // Button -> 5(SCK)
    auto display = std::make_unique<DisplayTarget>();
    auto gps_uart = std::make_unique<UartGps>(UART_NUM_1,
                                              17,  // RX -> A0
                                              16); // TX -> A1


    // Threads
    auto storage = std::make_unique<Storage>(*target_nvm, state);
    auto producer = std::make_unique<TileProducer>(*map_metadata);
    auto route_service = std::make_unique<RouteService>(*map_metadata);
    auto gps_simulator = std::make_unique<GpsSimulator>(*map_metadata, state, *route_service);

    // Selects between the real and demo GPS
    auto gps_mux = std::make_unique<GpsMux>(state, *gps_uart, *gps_simulator);

    auto gps_reader = std::make_unique<GpsReader>(*map_metadata, *gps_mux);
    auto ui = std::make_unique<UserInterface>(state,
                                              *map_metadata,
                                              *producer,
                                              *display,
                                              *encoder_input,
                                              *route_service,
                                              gps_reader->AttachListener(),
                                              route_service->AttachListener());

    storage->Start(1);
    gps_simulator->Start(1);
    gps_reader->Start(1);
    producer->Start(0, os::ThreadPriority::kHigh);
    route_service->Start(1, os::ThreadPriority::kNormal, 5000);
    ui->Start(0, os::ThreadPriority::kHigh, 8192);

    while (true)
    {
        vTaskSuspend(nullptr);
    }
}
