#include "mainwindow.hh"

#include "i_display.hh"
#include "tile_producer.hh"
#include "ui_mainwindow.h"

#include "gps_reader.hh"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_ui(new Ui::MainWindow)
{
    m_ui->setupUi(this);

    m_scene = std::make_unique<QGraphicsScene>();
    m_display = std::make_unique<DisplayQt>(m_scene.get());
    m_ui->displayGraphicsView->setScene(m_scene.get());

    // All this is TMP
    auto gps_reader = GpsReader();
    auto producer = TileProducer(gps_reader.AttachListener());

    gps_reader.Start();
    producer.Start();

    auto t0 = producer.LockTile(0, 0);
    auto t1 = producer.LockTile(240, 0);
    auto t11 = producer.LockTile(0, 240);
    auto t12 = producer.LockTile(240, 240);

    if (t0 && t1 && t11 && t12)
    {
        m_display->Blit(t0->GetImage(), Rect{0, 0});
        m_display->Blit(t1->GetImage(), Rect{240, 0});
        m_display->Blit(t11->GetImage(), Rect{0, 240});
        m_display->Blit(t12->GetImage(), Rect{240, 240});
    }
    m_display->Flip();
}

MainWindow::~MainWindow()
{
    delete m_ui;
}
