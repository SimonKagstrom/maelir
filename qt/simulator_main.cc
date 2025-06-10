#include "gps_listener.hh"
#include "gps_reader.hh"
#include "gps_simulator.hh"
#include "nvm_host.hh"
#include "ota_updater.hh"
#include "ota_updater_host.hh"
#include "route_service.hh"
#include "route_utils.hh"
#include "simulator_mainwindow.hh"
#include "storage.hh"
#include "tile_producer.hh"
#include "time.hh"
#include "trip_computer.hh"
#include "uart_bridge.hh"
#include "uart_event_forwarder.hh"
#include "uart_event_listener.hh"
#include "ui.hh"

#include <QApplication>
#include <QCommandLineParser>
#include <QFile>
#include <fmt/format.h>
#include <stdlib.h>

int
main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    QCommandLineParser parser;

    parser.addOptions({
        {{"s", "seed"}, "Random seed", "seed"},
        {{"u", "updated"}, "Set the application updated flag"},
        {{"m", "map"}, "Path to the map file", "map_file"},
    });

    parser.process(a);

    QString map_file = "map.bin";
    if (parser.isSet("map"))
    {
        map_file = parser.value("map");
    }
    int seed = 0;
    if (parser.isSet("seed"))
    {
        seed = parser.value("seed").toInt();
    }
    bool updated = false;
    if (parser.isSet("updated"))
    {
        updated = true;
    }

    MainWindow window;

    auto bin_file = QFile(map_file);
    auto rv = bin_file.open(QIODevice::ReadOnly);
    if (!rv)
    {
        fmt::print("Failed to open {}\n", map_file.toStdString());
        return 1;
    }

    auto mmap_bin = bin_file.map(0, bin_file.size());
    if (!mmap_bin)
    {
        fmt::print("Failed to map {}\n", map_file.toStdString());
        return 1;
    }

    auto nvm = std::make_unique<NvmHost>("nvm.txt");

    ApplicationState state;

    srand(seed);
    state.Checkout()->demo_mode = true;

    auto map_metadata = reinterpret_cast<const MapMetadata*>(mmap_bin);

    fmt::print("Metadata @ {}..{}:\n  {}x{} tiles\n  {}x{} land mask\n  {}x{} GPS data\n  0x{:x} "
               "tile_data_offset\n  0x{:x}  land_mask_data_offset\n  0x{:x} "
               "gps_position_offset\n  latitude between {}..{}\n  longitude between {}..{}\n",
               (const void*)map_metadata,
               (const void*)((const uint8_t*)map_metadata + bin_file.size()),
               map_metadata->tile_row_size,
               map_metadata->tile_rows,
               map_metadata->land_mask_row_size,
               map_metadata->land_mask_rows,
               map_metadata->gps_data_row_size,
               map_metadata->gps_data_rows,
               map_metadata->tile_data_offset,
               map_metadata->land_mask_data_offset,
               map_metadata->gps_position_offset,

               map_metadata->lowest_latitude,
               map_metadata->highest_latitude,
               map_metadata->lowest_longitude,
               map_metadata->highest_longitude);

    auto route_service = std::make_unique<RouteService>(*map_metadata);
    auto storage = std::make_unique<Storage>(*nvm, state, route_service->AttachListener());
    auto producer = std::make_unique<TileProducer>(state, *map_metadata);
    auto gps_simulator = std::make_unique<GpsSimulator>(*map_metadata, state, *route_service);
    auto gps_listener = std::make_unique<GpsListener>(*gps_simulator);


    auto ota_updater_device = std::make_unique<OtaUpdaterHost>(updated);

    auto uart_bridge = std::make_unique<UartBridge>();
    auto [uart_a, uart_b] = uart_bridge->GetEndpoints();

    auto uart_event_listener = std::make_unique<UartEventListener>(uart_a);
    auto uart_event_forwarder = std::make_unique<UartEventForwarder>(uart_b, window, *gps_listener);
    auto gps_reader = std::make_unique<GpsReader>(*map_metadata, *uart_event_listener);
    auto trip_computer = std::make_unique<TripComputer>(state,
                                                        gps_reader->AttachListener(),
                                                        route_service->AttachListener(),
                                                        LookupMetersPerPixel(*map_metadata),
                                                        map_metadata->land_mask_row_size);


    auto ota_updater = std::make_unique<OtaUpdater>(*ota_updater_device, state);

    auto ui = std::make_unique<UserInterface>(state,
                                              *map_metadata,
                                              *producer,
                                              window.GetDisplay(),
                                              *uart_event_listener,
                                              *route_service,
                                              *ota_updater,
                                              gps_reader->AttachListener(),
                                              route_service->AttachListener());


    storage->Start("storage");

    // Hack to make sure storage has started (not needed with FreeRTOS)
    os::Sleep(10ms);

    gps_listener->Start("gps_listener");
    uart_event_listener->Start("uart_event_listener");
    uart_event_forwarder->Start("uart_event_forwarder");

    gps_simulator->Start("gps_simulator");
    gps_reader->Start("gps_reader");
    producer->Start("tile_producer");
    route_service->Start("route_service");
    trip_computer->Start("trip_computer");
    ota_updater->Start("ota_updater");
    ui->Start("ui");

    window.show();

    //    route_service->RequestRoute({2592, 7032}, {16008, 4728});
    //route_service->RequestRoute({8735,6117}, {9771, 6493});
    auto out = QApplication::exec();

    // Workaround a hang on exit. The target application never exits
    exit(0);

    return out;
}
