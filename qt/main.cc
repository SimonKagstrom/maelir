#include "mainwindow.hh"

#include <QApplication>
#include <QFile>
#include <fmt/format.h>
#include <stdlib.h>

int
main(int argc, char* argv[])
{
    QApplication a(argc, argv);


    MainWindow w;

    w.show();

    return QApplication::exec();
}
