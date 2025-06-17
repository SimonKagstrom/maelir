#include "mapeditor_mainwindow.hh"

#include <QApplication>
#include <print>
#include <stdlib.h>

int
main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::print("Usage: {} <map_data_yaml>\n", argv[0]);
        return 1;
    }

    QApplication a(argc, argv);

    MapEditorMainWindow window(argv[1]);

    window.show();

    return QApplication::exec();
}
