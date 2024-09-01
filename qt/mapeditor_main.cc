#include "mapeditor_mainwindow.hh"

#include <QApplication>
#include <fmt/format.h>
#include <stdlib.h>

int
main(int argc, char* argv[])
{
    if (argc < 3)
    {
        fmt::print("Usage: {} <map_png> <out_headerfile>\n", argv[0]);
        return 1;
    }

    QApplication a(argc, argv);

    auto map = std::make_unique<QImage>(argv[1]);
    if (map->isNull())
    {
        fmt::print("Failed to load image: {}\n", argv[1]);
        return 1;
    }
    auto out_file = std::make_unique<QFile>(argv[2]);
    if (!out_file->open(QIODevice::WriteOnly | QIODevice::Text))
    {
        fmt::print("Failed to open file: {}\n", argv[2]);
        return 1;
    }

    MapEditorMainWindow window(std::move(map), std::move(out_file));

    window.show();

    return QApplication::exec();
}
