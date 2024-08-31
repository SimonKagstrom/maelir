#include "mapeditor_mainwindow.hh"

#include "ui_mapeditor_mainwindow.h"

MapEditorMainWindow::MapEditorMainWindow(std::unique_ptr<QImage> map, QWidget* parent)
    : QMainWindow(parent)
    , m_ui(new Ui::MainWindow)
    , m_map(std::move(map))
    , m_scene(std::make_unique<QGraphicsScene>())
    , m_pixmap(m_scene->addPixmap(QPixmap::fromImage(*m_map)))
{
    m_ui->setupUi(this);

    m_ui->displayGraphicsView->setScene(m_scene.get());
    m_ui->displayGraphicsView->centerOn(0, 0);


    // Install the event filter on the viewport
    m_ui->displayGraphicsView->viewport()->installEventFilter(this);
}

MapEditorMainWindow::~MapEditorMainWindow()
{
    delete m_ui;
}


bool
MapEditorMainWindow::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == m_ui->displayGraphicsView->viewport() && filterMouse(obj, event))
    {
        return true;
    }

    return QMainWindow::eventFilter(obj, event);
}

bool
MapEditorMainWindow::filterMouse(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton)
        {
            m_panning = true;
            m_lastMousePos = mouseEvent->pos();
            setCursor(Qt::ClosedHandCursor);
            return true;
        }
    }
    else if (event->type() == QEvent::MouseMove)
    {
        if (m_panning)
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            QPoint delta = mouseEvent->pos() - m_lastMousePos;
            m_lastMousePos = mouseEvent->pos();
            m_ui->displayGraphicsView->centerOn(
                m_ui->displayGraphicsView->mapToScene(
                    m_ui->displayGraphicsView->viewport()->rect().center()) -
                delta);
            return true;
        }
    }
    else if (event->type() == QEvent::MouseButtonRelease)
    {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton)
        {
            m_panning = false;
            setCursor(Qt::ArrowCursor);
            return true;
        }
    }

    return false;
}