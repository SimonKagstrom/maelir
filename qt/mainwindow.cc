#include "mainwindow.hh"

#include "i_display.hh"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_ui(new Ui::MainWindow)
{
    m_ui->setupUi(this);

    m_scene = std::make_unique<QGraphicsScene>();
}

MainWindow::~MainWindow()
{
    delete m_ui;
}
