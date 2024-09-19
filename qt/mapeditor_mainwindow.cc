// A map editor, which is a bit of a hack
#include "mapeditor_mainwindow.hh"

#include "tile.hh"
#include "ui_mapeditor_mainwindow.h"

#include <QImageReader>
#include <QInputDialog>
#include <QMessageBox>
#include <fmt/format.h>
#include <fstream>

MapEditorMainWindow::MapEditorMainWindow(const QString& map_name,
                                         const QString& out_yaml,
                                         QWidget* parent)
    : QMainWindow(parent)
    , m_ui(new Ui::MainWindow)
    , m_map_name(map_name)
    , m_out_yaml(out_yaml)
    , m_scene(std::make_unique<QGraphicsScene>())
{
    QImageReader::setAllocationLimit(4096);

    m_map = std::make_unique<QImage>(map_name);
    if (m_map->isNull())
    {
        fmt::print("Failed to load image: {}\n", map_name.toStdString());
        exit(1);
    }

    auto cropped_height = m_map->height() - m_map->height() % kTileSize;
    auto cropped_width = m_map->width() - m_map->width() % kTileSize;

    *m_map = m_map->copy(0, 0, cropped_width, cropped_height);

    m_land_mask.resize(
        (cropped_height / kPathFinderTileSize) * (cropped_width / kPathFinderTileSize), false);
    m_pixmap = m_scene->addPixmap(QPixmap::fromImage(*m_map));

    m_ui->setupUi(this);
    m_ui->displayGraphicsView->SetOwner(this);

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
            auto filtered = FilterMouse(obj, event);
            if (filtered)
            {
                m_scene->invalidate(m_scene->sceneRect());
            }

            return filtered;
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
    auto action_add_extra_land = contextMenu.addAction("Mark this route tile as land");
    auto action_add_extra_water = contextMenu.addAction("Mark this route tile as water");
    auto action_route_add_point = contextMenu.addAction("Add route point");
    // Add delimiter
    contextMenu.addSeparator();
    auto action_set_coordinates = contextMenu.addAction("Set GPS coordinates for this point");
    auto action_add_land_color = contextMenu.addAction("Add land color for this point");
    auto action_calculate_land = contextMenu.addAction("Calculate land tiles");

    auto selectedAction = contextMenu.exec(mouse_position);
    auto [x, y] = GetMapCoordinates(map_posititon);

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

    else if (selectedAction == action_add_land_color)
    {
        auto color = m_map->pixelColor(x, y);
        fmt::print("Color at {},{}: R: {}, G: {}, B: {}\n",
                   x,
                   y,
                   color.red(),
                   color.green(),
                   color.blue());

        m_land_colors.insert(color);
    }
    else if (selectedAction == action_calculate_land)
    {
        CalculateLand();
        UpdateRoutingInformation();
    }
    else if (selectedAction == action_add_extra_land)
    {
        AddExtraLand(x, y);
    }
    else if (selectedAction == action_add_extra_water)
    {
        AddExtraWater(x, y);
    }
    else if (selectedAction == action_route_add_point)
    {
        if (m_wanted_route.size() == 2)
        {
            m_wanted_route.clear();
            m_current_route = {};
        }
        m_wanted_route.push_back(Point {x, y});

        if (m_wanted_route.size() == 2 && m_router)
        {
            auto from = m_wanted_route.front();
            auto to = m_wanted_route.back();

            m_current_route = m_router->CalculateRoute(from, to);
            if (!m_current_route.empty())
            {
                auto stats = m_router->GetStats();
                fmt::print(
                    "Route from {},{} to {},{} with {} expanded nodes for {} partial paths\n",
                    from.x,
                    from.y,
                    to.x,
                    to.y,
                    stats.nodes_expanded,
                    stats.partial_paths);
            }
        }
    }
    update();
    m_ui->displayGraphicsView->repaint();
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

        for (const auto& pos : node["extra_land"])
        {
            if (pos["x_pixel"] && pos["y_pixel"])
            {
                m_extra_land.insert({pos["x_pixel"].as<int>(), pos["y_pixel"].as<int>()});
            }
        }

        for (const auto& pos : node["extra_water"])
        {
            if (pos["x_pixel"] && pos["y_pixel"])
            {
                m_extra_water.insert({pos["x_pixel"].as<int>(), pos["y_pixel"].as<int>()});
            }
        }

        auto index = 0;
        for (const auto& mask_uint32 : node["land_mask"])
        {
            auto val = mask_uint32.as<uint32_t>();

            if (index >= m_land_mask.size())
            {
                break;
            }

            for (auto i = 0; i < 32; ++i)
            {
                m_land_mask[index] = val & (1 << i);
                index++;
            }
        }

        UpdateRoutingInformation();
    } catch (const YAML::BadFile& e)
    {
        // Just ignore this
    }
}

void
MapEditorMainWindow::SaveYaml()
{
    YAML::Node node;

    // Globals
    node["map_filename"] = m_map_name.toStdString();
    node["tile_size"] = kTileSize;
    node["path_finder_tile_size"] = kPathFinderTileSize;

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

    if (!m_land_colors.empty())
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

    if (!m_extra_land.empty())
    {
        YAML::Node extra_land;
        for (const auto& pos : m_extra_land)
        {
            YAML::Node pos_node;

            pos_node["x_pixel"] = pos.first;
            pos_node["y_pixel"] = pos.second;

            extra_land.push_back(pos_node);
        }

        node["extra_land"] = extra_land;
    }
    if (!m_extra_water.empty())
    {
        YAML::Node extra_water;
        for (const auto& pos : m_extra_water)
        {
            YAML::Node pos_node;

            pos_node["x_pixel"] = pos.first;
            pos_node["y_pixel"] = pos.second;

            extra_water.push_back(pos_node);
        }

        node["extra_water"] = extra_water;
    }


    node["land_mask"] = m_land_mask_uint32;
    UpdateRoutingInformation();

    std::ofstream f(m_out_yaml.toStdString());

    f << node;
}

void
MapEditorMainWindow::CalculateLand()
{
    if (m_land_colors.empty())
    {
        return;
    }

    for (auto x = 0; x < m_map->width(); x += kPathFinderTileSize)
    {
        for (auto y = 0; y < m_map->height(); y += kPathFinderTileSize)
        {
            auto chunk = m_map->copy(x, y, kPathFinderTileSize, kPathFinderTileSize);

            // it's land if it contains at least 20% land colors
            if (CountLandPixels(chunk) > 0.2 * kPathFinderTileSize * kPathFinderTileSize)
            {
                m_land_mask[(y / kPathFinderTileSize) * m_map->width() / kPathFinderTileSize +
                            x / kPathFinderTileSize] = true;
            }
        }
    }
    printf("Land mask calculated for %d,%d -> %d,%d\n",
           m_map->width(),
           m_map->height(),
           m_map->width() / kPathFinderTileSize,
           m_map->height() / kPathFinderTileSize);
    UpdateRoutingInformation();
}

void
MapEditorMainWindow::UpdateRoutingInformation()
{
    for (auto [x, y] : m_extra_land)
    {
        AddExtraLand(x, y);
    }

    for (auto [x, y] : m_extra_water)
    {
        AddExtraWater(x, y);
    }

    m_land_mask_uint32.clear();
    // Save land mask as uint32_t values
    uint32_t cur_val = 0;
    int i;
    for (i = 0; i < m_land_mask.size(); ++i)
    {
        cur_val |= m_land_mask[i] << (i % 32);
        if ((i + 1) % 32 == 0)
        {
            m_land_mask_uint32.push_back(cur_val);
            cur_val = 0;
        }
    }
    if (m_land_mask.size() % 32 != 0)
    {
        // Fill the remaining bits with 1s (as land)
        cur_val |= 0xffffffff >> (i % 32);
        m_land_mask_uint32.push_back(cur_val);
    }

    m_router = std::make_unique<Router<kTargetCacheSize>>(m_land_mask_uint32,
                                                          m_map->height() / kPathFinderTileSize,
                                                          m_map->width() / kPathFinderTileSize);
}

void
MapEditorMainWindow::AddExtraLand(int x, int y)
{
    x = x - x % kPathFinderTileSize;
    y = y - y % kPathFinderTileSize;

    m_extra_land.insert({x, y});
    m_extra_water.erase({x, y});
    m_land_mask[(y / kPathFinderTileSize) * m_map->width() / kPathFinderTileSize +
                x / kPathFinderTileSize] = true;
    m_scene->addRect(x, y, kPathFinderTileSize, kPathFinderTileSize, QPen(Qt::green));
}

void
MapEditorMainWindow::AddExtraWater(int x, int y)
{
    x = x - x % kPathFinderTileSize;
    y = y - y % kPathFinderTileSize;

    m_extra_water.insert({x, y});
    m_extra_land.erase({x, y});
    m_land_mask[(y / kPathFinderTileSize) * m_map->width() / kPathFinderTileSize +
                x / kPathFinderTileSize] = false;
    m_scene->addRect(x, y, kPathFinderTileSize, kPathFinderTileSize, QPen(Qt::cyan));
}


unsigned
MapEditorMainWindow::CountLandPixels(QImage& chunk)
{
    auto land = 0u;

    for (auto x = 0; x < chunk.width(); ++x)
    {
        for (auto y = 0; y < chunk.height(); ++y)
        {
            auto color = chunk.pixelColor(x, y);
            if (m_land_colors.find(color) != m_land_colors.end())
            {
                ++land;
            }
        }
    }

    return land;
}
