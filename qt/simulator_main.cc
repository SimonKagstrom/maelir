#include "gps_reader.hh"
#include "gps_simulator.hh"
#include "route_service.hh"
#include "simulator_mainwindow.hh"
#include "tile_producer.hh"
#include "time.hh"
#include "ui.hh"

#include <QApplication>
#include <QFile>
#include <fmt/format.h>
#include <stdlib.h>

int
main(int argc, char* argv[])
{
    if (argc != 2)
    {
        fmt::print("Usage: {} <path_to_generated_tiles.bin>\n", argv[0]);
        exit(1);
    }

    QApplication a(argc, argv);
    MainWindow window;

    auto bin_file = QFile(argv[1]);
    auto rv = bin_file.open(QIODevice::ReadOnly);
    if (!rv)
    {
        fmt::print("Failed to open {}\n", argv[1]);
        return 1;
    }

    auto mmap_bin = bin_file.map(0, bin_file.size());
    if (!mmap_bin)
    {
        fmt::print("Failed to map {}\n", argv[1]);
        return 1;
    }

    ApplicationState state;

    srand(time(0));
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

    auto producer = std::make_unique<TileProducer>(*map_metadata);
    auto route_service = std::make_unique<RouteService>(*map_metadata);
    auto gps_simulator = std::make_unique<GpsSimulator>(*map_metadata, state, *route_service);
    auto gps_reader = std::make_unique<GpsReader>(*map_metadata, *gps_simulator);

    auto ui = std::make_unique<UserInterface>(state,
                                              *map_metadata,
                                              *producer,
                                              window.GetDisplay(),
                                              window,
                                              gps_reader->AttachListener(),
                                              route_service->AttachListener());


    gps_simulator->Start();
    gps_reader->Start();
    producer->Start();
    route_service->Start();
    ui->Start();

    window.show();

    //    route_service->RequestRoute({2592, 7032}, {16008, 4728});
    //route_service->RequestRoute({8735,6117}, {9771, 6493});
    auto out = QApplication::exec();

    // Stop to avoid the destructor accessing the thread
    ui->Stop();
    route_service->Stop();
    producer->Stop();
    gps_reader->Stop();
    gps_simulator->Stop();

    return out;
}
