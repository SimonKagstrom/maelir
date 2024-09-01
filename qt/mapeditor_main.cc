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

    MapEditorMainWindow window(argv[1], argv[2]);

    window.show();

    return QApplication::exec();
}
