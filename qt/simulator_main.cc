#include "gps_reader.hh"
#include "gps_simulator.hh"
#include "route_service.hh"
#include "simulator_mainwindow.hh"
#include "tile_producer.hh"
#include "ui.hh"

#include <QApplication>
#include <QFile>
#include <fmt/format.h>
#include <stdlib.h>

int
main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    MainWindow window;

    if (argc != 2)
    {
        fmt::print("Usage: {} <path_to_generated_tiles.bin>\n", argv[0]);
    }

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

    auto gps_simulator = std::make_unique<GpsSimulator>();
    auto gps_reader = std::make_unique<GpsReader>(*gps_simulator);
    auto producer = std::make_unique<TileProducer>(reinterpret_cast<const FlashTileMetadata*>(mmap_bin));
    auto route_service = std::make_unique<RouteService>();

    auto ui = std::make_unique<UserInterface>(*producer,
                                              window.GetDisplay(),
                                              gps_reader->AttachListener(),
                                              route_service->AttachListener());

    gps_simulator->Start();
    gps_reader->Start();
    producer->Start();
    route_service->Start();
    ui->Start();

    window.show();

    route_service->RequestRoute({678, 865}, {1844, 777});

    auto out = QApplication::exec();

    // Stop to avoid the destructor accessing the thread
    ui->Stop();
    route_service->Stop();
    producer->Stop();
    gps_reader->Stop();
    gps_simulator->Stop();

    return out;
}
