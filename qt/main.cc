#include "gps_reader.hh"
#include "mainwindow.hh"
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

    auto gps_reader = std::make_unique<GpsReader>();
    auto producer = std::make_unique<TileProducer>(gps_reader->AttachListener());
    auto ui = std::make_unique<UserInterface>(
        *producer, window.GetDisplay(), gps_reader->AttachListener());

    gps_reader->Start();
    producer->Start();
    ui->Start();

    window.show();

    return QApplication::exec();
}
