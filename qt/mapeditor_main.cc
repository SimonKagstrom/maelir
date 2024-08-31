#include "mapeditor_mainwindow.hh"

#include <QApplication>
#include <QFile>
#include <fmt/format.h>
#include <stdlib.h>

int
main(int argc, char* argv[])
{
    if (argc < 2)
    {
        fmt::print("Usage: {} <map_png>\n", argv[0]);
        return 1;
    }

    QApplication a(argc, argv);

    auto map = std::make_unique<QImage>(argv[1]);
    if (map->isNull())
    {
        fmt::print("Failed to load image: {}\n", argv[1]);
        return 1;
    }


    MapEditorMainWindow window(std::move(map));

    window.show();

    return QApplication::exec();
}
