#include "mapeditor_mainwindow.hh"

#include "ui_mapeditor_mainwindow.h"

#include <QInputDialog>
#include <QMessageBox>
#include <fmt/format.h>

MapEditorMainWindow::MapEditorMainWindow(std::unique_ptr<QImage> map,
                                         std::unique_ptr<QFile> out_header_file,
                                         QWidget* parent)
    : QMainWindow(parent)
    , m_ui(new Ui::MainWindow)
    , m_map(std::move(map))
    , out_header_file(std::move(out_header_file))
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
    if (obj == m_ui->displayGraphicsView->viewport())
    {
        if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseMove ||
            event->type() == QEvent::MouseButtonRelease)
        {
            return FilterMouse(obj, event);
        }
    }

    return QMainWindow::eventFilter(obj, event);
}

bool
MapEditorMainWindow::FilterMouse(QObject* obj, QEvent* event)
{
    QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);

    if (event->type() == QEvent::MouseButtonPress)
    {
        m_last_mouse_pos = mouse_event->pos();
        if (mouse_event->button() == Qt::LeftButton)
        {
            m_panning = true;
            setCursor(Qt::ClosedHandCursor);
            return true;
        }
        else if (mouse_event->button() == Qt::RightButton)
        {
            RightClickContextMenu(mouse_event->globalPosition().toPoint(), m_last_mouse_pos);
            return true;
        }
    }
    else if (event->type() == QEvent::MouseMove)
    {
        auto [x, y] = GetMapCoordinates(mouse_event->pos());
        m_ui->coordinateLabel->setText(QString("x: %1, y: %2").arg(x).arg(y));

        if (m_panning)
        {
            QPoint delta = mouse_event->pos() - m_last_mouse_pos;

            m_last_mouse_pos = mouse_event->pos();
            m_ui->displayGraphicsView->centerOn(
                m_ui->displayGraphicsView->mapToScene(
                    m_ui->displayGraphicsView->viewport()->rect().center()) -
                delta);
            return true;
        }
    }
    else if (event->type() == QEvent::MouseButtonRelease)
    {
        if (mouse_event->button() == Qt::LeftButton)
        {
            m_panning = false;
            setCursor(Qt::ArrowCursor);
            return true;
        }
    }

    return false;
}

std::pair<int, int>
MapEditorMainWindow::GetMapCoordinates(QPoint pos)
{
    QPointF scenePos = m_ui->displayGraphicsView->mapToScene(pos);

    return {static_cast<int>(scenePos.x()), static_cast<int>(scenePos.y())};
}

void
MapEditorMainWindow::RightClickContextMenu(QPoint mouse_position, QPoint map_posititon)
{
    // Show context menu
    QMenu contextMenu(this);
    QAction* action_set_coordinates = contextMenu.addAction("Set GPS coordinates for this point");
    QAction* selectedAction = contextMenu.exec(mouse_position);

    if (selectedAction == action_set_coordinates)
    {
        bool ok;
        QString text = QInputDialog::getText(
            this,
            tr("Set GPS Coordinates"),
            tr("Enter Latitude, longitude (e.g., 59.30233189848152, 17.941052011787928):"),
            QLineEdit::Normal,
            "",
            &ok);

        if (ok && !text.isEmpty())
        {
            QStringList coordinates = text.split(",");
            if (coordinates.size() == 2)
            {
                bool long_ok, lat_ok;
                double latitude = coordinates[0].trimmed().toDouble(&long_ok);
                double longitude = coordinates[1].trimmed().toDouble(&lat_ok);

                if (long_ok && lat_ok)
                {
                    auto [x, y] = GetMapCoordinates(map_posititon);
                    SetGpsPosition(longitude, latitude, x, y);
                }
                else
                {
                    QMessageBox::warning(
                        this,
                        tr("Invalid Input"),
                        tr("Please enter valid numeric values for longitude and latitude."));
                }
            }
            else
            {
                QMessageBox::warning(
                    this,
                    tr("Invalid Input"),
                    tr("Please enter the coordinates in the format: Longitude, Latitude."));
            }
        }
    }
}

void
MapEditorMainWindow::SetGpsPosition(double longitude, double latitude, int x, int y)
{
    fmt::print("Setting GPS coordinates at {},{} to Longitude: {}, Latitude: {}\n",
               x,
               y,
               longitude,
               latitude);

    if (m_positions.full())
    {
        m_positions.pop_front();
    }
    m_positions.push_back({longitude, latitude, x, y});

    if (m_positions.size() == 2)
    {
        auto& first = m_positions.front();
        auto& second = m_positions.back();

        auto longitude_diff = second.longitude - first.longitude;
        auto latitude_diff = second.latitude - first.latitude;

        auto x_pixel_diff = second.x - first.x;
        auto y_pixel_diff = second.y - first.y;

        fmt::print("Longitude diff: {}, Latitude diff: {}\n", longitude_diff, latitude_diff);
        fmt::print("X pixel diff: {}, Y pixel diff: {}\n", x_pixel_diff, y_pixel_diff);
    }
}
