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

    auto gps_simulator = std::make_unique<GpsSimulator>();
    auto gps_reader = std::make_unique<GpsReader>(*gps_simulator);
    auto producer = std::make_unique<TileProducer>();
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
