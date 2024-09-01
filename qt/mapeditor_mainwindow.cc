#include "mapeditor_mainwindow.hh"

#include "ui_mapeditor_mainwindow.h"

#include <QInputDialog>
#include <QMessageBox>
#include <fmt/format.h>
#include <fstream>

MapEditorMainWindow::MapEditorMainWindow(std::unique_ptr<QImage> map,
                                         const QString& map_name,
                                         const QString& out_yaml,
                                         QWidget* parent)
    : QMainWindow(parent)
    , m_ui(new Ui::MainWindow)
    , m_map(std::move(map))
    , m_map_name(map_name)
    , m_out_yaml(out_yaml)
    , m_scene(std::make_unique<QGraphicsScene>())
    , m_pixmap(m_scene->addPixmap(QPixmap::fromImage(*m_map)))
{
    m_ui->setupUi(this);

    m_ui->displayGraphicsView->setScene(m_scene.get());
    m_ui->displayGraphicsView->centerOn(0, 0);

    // Install the event filter on the viewport
    m_ui->displayGraphicsView->viewport()->installEventFilter(this);

    LoadYaml(m_out_yaml.toStdString().c_str());
}

MapEditorMainWindow::~MapEditorMainWindow()
{
    SaveYaml();
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
        QString gps_text = "";
        if (m_map_position_data)
        {
            auto longitude = m_map_position_data->longitude_at_pixel_0 +
                             m_map_position_data->longitude_per_pixel * x;
            auto latitude = m_map_position_data->latitude_at_pixel_0 +
                            m_map_position_data->latitude_per_pixel * y;

            gps_text = QString(",  Longitude: %1, Latitude: %2").arg(longitude).arg(latitude);
        }

        m_ui->coordinateLabel->setText(QString("x: %1, y: %2%3").arg(x).arg(y).arg(gps_text));

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
    auto action_set_coordinates = contextMenu.addAction("Set GPS coordinates for this point");
    auto action_add_land_color = contextMenu.addAction("Add land color for this point");
    auto selectedAction = contextMenu.exec(mouse_position);

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

                    SaveYaml();
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

    if (selectedAction == action_add_land_color)
    {
        auto [x, y] = GetMapCoordinates(map_posititon);
        auto color = m_map->pixelColor(x, y);
        fmt::print("Color at {},{}: R: {}, G: {}, B: {}\n",
                   x,
                   y,
                   color.red(),
                   color.green(),
                   color.blue());

        m_land_colors.insert(color);
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
        m_positions.clear();
        m_map_position_data = std::nullopt;
    }
    m_positions.push_back({longitude, latitude, x, y});

    QVector<QPointF> scene_positions;
    for (auto pos : m_positions)
    {
        scene_positions.push_back(QPointF(pos.x, pos.y));
    }
    m_ui->displayGraphicsView->SetGpsPositions(scene_positions);

    if (m_positions.size() == 2)
    {
        auto& first = m_positions.front();
        auto& second = m_positions.back();

        if (first.x > second.x)
        {
            printf(
                "First coordinate should be top left, second bottom right. Unreliable results\n");
            std::swap(first, second);
        }

        auto longitude_diff = second.longitude - first.longitude;
        auto latitude_diff = second.latitude - first.latitude;

        auto x_pixel_diff = second.x - first.x;
        auto y_pixel_diff = second.y - first.y;

        auto latitude_at_pixel_0 = first.latitude - (latitude_diff / y_pixel_diff) * first.y;
        auto longitude_at_pixel_0 = first.longitude - (longitude_diff / x_pixel_diff) * first.x;

        auto latitude_pixel_size = y_pixel_diff / latitude_diff;
        auto longitude_pixel_size = x_pixel_diff / longitude_diff;

        m_map_position_data = {longitude_at_pixel_0,
                               latitude_at_pixel_0,
                               longitude_diff / x_pixel_diff,
                               latitude_diff / y_pixel_diff,
                               static_cast<unsigned>(std::abs(longitude_pixel_size)),
                               static_cast<unsigned>(std::abs(latitude_pixel_size))};

        fmt::print("Longitude diff: {}, Latitude diff: {}\n", longitude_diff, latitude_diff);
        fmt::print("X pixel diff: {}, Y pixel diff: {}\n", x_pixel_diff, y_pixel_diff);
        fmt::print("Latitude at pixel 0: {}, Longitude at pixel 0: {}\n",
                   latitude_at_pixel_0,
                   longitude_at_pixel_0);
    }

    update();
}

void
MapEditorMainWindow::LoadYaml(const char* filename)
{
    try
    {
        auto node = YAML::LoadFile(filename);

        for (const auto& pos : node["reference_positions"])
        {
            if (pos["longitude"] && pos["latitude"] && pos["x_pixel"] && pos["y_pixel"])
            {
                SetGpsPosition(pos["longitude"].as<double>(),
                               pos["latitude"].as<double>(),
                               pos["x_pixel"].as<int>(),
                               pos["y_pixel"].as<int>());
            }
        }

        for (const auto& color : node["land_pixel_colors"])
        {
            if (color["r"] && color["g"] && color["b"])
            {
                m_land_colors.insert(
                    QColor(color["r"].as<int>(), color["g"].as<int>(), color["b"].as<int>()));
            }
        }
    } catch (const YAML::BadFile& e)
    {
        // Just ignore this
    }
}

void
MapEditorMainWindow::SaveYaml()
{
    YAML::Node node;

    node["map_filename"] = m_map_name.toStdString();
    for (const auto& pos : m_positions)
    {
        YAML::Node pos_node;

        pos_node["longitude"] = pos.longitude;
        pos_node["latitude"] = pos.latitude;
        pos_node["x_pixel"] = pos.x;
        pos_node["y_pixel"] = pos.y;

        node["reference_positions"].push_back(pos_node);
    }

    if (m_map_position_data)
    {
        YAML::Node corner_position;

        corner_position["latitude"] = m_map_position_data->latitude_at_pixel_0;
        corner_position["longitude"] = m_map_position_data->longitude_at_pixel_0;
        corner_position["latitude_pixel_size"] = m_map_position_data->latitude_pixel_size;
        corner_position["longitude_pixel_size"] = m_map_position_data->longitude_pixel_size;

        node["corner_position"] = corner_position;
    }

    if (m_land_colors.size() > 0)
    {
        YAML::Node colors;
        for (const auto& color : m_land_colors)
        {
            YAML::Node color_node;

            color_node["r"] = color.red();
            color_node["g"] = color.green();
            color_node["b"] = color.blue();

            colors.push_back(color_node);
        }

        node["land_pixel_colors"] = colors;
    }

    std::ofstream f(m_out_yaml.toStdString());

    f << node;
}
