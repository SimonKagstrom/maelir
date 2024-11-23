// A map editor, which is a bit of a hack
#include "mapeditor_mainwindow.hh"

#include "tile.hh"
#include "ui_mapeditor_mainwindow.h"

#include <QImageReader>
#include <QInputDialog>
#include <QMessageBox>
#include <fmt/format.h>
#include <fstream>

namespace
{

GpsPosition
InterpolateGpsPosition(std::span<const MapGpsRasterTile> positions, auto map_width, auto x, auto y)
{
    auto index = y / kTileSize * map_width / kTileSize + x / kTileSize;
    auto bottom_right_index = index + 1 - map_width / kTileSize;

    if (bottom_right_index >= positions.size() || bottom_right_index < 0)
    {
        return GpsPosition {.latitude = positions[index].latitude,
                            .longitude = positions[index].longitude};
    }
    if (index >= positions.size())
    {
        // Should never happen
        return GpsPosition {.latitude = positions[index].latitude,
                            .longitude = positions[index].longitude};
    }

    auto longitude_difference =
        positions[bottom_right_index].longitude - positions[index].longitude;
    auto latitude_difference = positions[bottom_right_index].latitude - positions[index].latitude;

    auto x_offset = x % kTileSize;
    auto y_offset = y % kTileSize;

    auto longitude = positions[index].longitude + longitude_difference / kTileSize * x_offset;
    auto latitude =
        positions[bottom_right_index].latitude - latitude_difference / kTileSize * y_offset;

    return GpsPosition {.latitude = latitude, .longitude = longitude};
}

} // namespace


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
    assert(m_map->width() == cropped_width);
    assert(m_map->height() == cropped_height);

    m_all_land_tiles.resize((cropped_height / kTileSize) * (cropped_width / kTileSize), false);
    m_land_mask.resize(
        (cropped_height / kPathFinderTileSize) * (cropped_width / kPathFinderTileSize), false);
    m_pixmap = m_scene->addPixmap(QPixmap::fromImage(*m_map));

    m_gps_positions.resize((cropped_height / kGpsPositionSize) *
                           (cropped_width / kGpsPositionSize));

    m_ui->setupUi(this);
    m_ui->displayGraphicsView->SetOwner(this);

    m_ui->displayGraphicsView->setScene(m_scene.get());
    m_ui->displayGraphicsView->centerOn(cropped_width / 2, cropped_height / 2);

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
        auto position = InterpolateGpsPosition(m_gps_positions, m_map->width(), x, y);
        gps_text = QString(", position: %1, %2").arg(position.latitude).arg(position.longitude);

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
    auto action_skip_tile = contextMenu.addAction("Don't use this image tile for generation");
    // Add delimiter
    contextMenu.addSeparator();
    auto action_route_add_point = contextMenu.addAction("Add route point");
    // .. and here
    contextMenu.addSeparator();
    auto action_add_land_color = contextMenu.addAction("Add land color for this point");
    auto action_home_position = contextMenu.addAction("Set home position at this point");
    auto action_calculate_land = contextMenu.addAction("Calculate land tiles");

    auto selectedAction = contextMenu.exec(mouse_position);
    auto [x, y] = GetMapCoordinates(map_posititon);


    if (selectedAction == action_add_land_color)
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
    else if (selectedAction == action_skip_tile)
    {
        AddSkipTile(x, y);
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
            auto stats = m_router->GetStats();
            if (!m_current_route.empty())
            {
                fmt::print(
                    "Route from {},{} to {},{} with {} expanded nodes for {} partial paths\n",
                    from.x,
                    from.y,
                    to.x,
                    to.y,
                    stats.nodes_expanded,
                    stats.partial_paths);
            }
            else
            {
                fmt::print("No route found between {},{} and {},{} with {} expanded nodes\n",
                           from.x,
                           from.y,
                           to.x,
                           to.y,
                           stats.nodes_expanded);
            }
        }
    }
    else if (selectedAction == action_home_position)
    {
        m_home_position = Point {x, y};
    }


    update();
    m_ui->displayGraphicsView->repaint();
}

void
MapEditorMainWindow::LoadYaml(const char* filename)
{
    try
    {
        auto node = YAML::LoadFile(filename);

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
                AddExtraLand(pos["x_pixel"].as<int>(), pos["y_pixel"].as<int>());
            }
        }

        for (const auto& pos : node["extra_water"])
        {
            if (pos["x_pixel"] && pos["y_pixel"])
            {
                AddExtraWater(pos["x_pixel"].as<int>(), pos["y_pixel"].as<int>());
            }
        }

        for (const auto& pos : node["skip_tiles"])
        {
            if (pos["x_pixel"] && pos["y_pixel"])
            {
                AddSkipTile(pos["x_pixel"].as<int>(), pos["y_pixel"].as<int>());
            }
        }

        for (const auto& pos : node["all_land_tiles"])
        {
            if (pos["x_pixel"] && pos["y_pixel"])
            {
                auto index = (pos["y_pixel"].as<int>() / kTileSize) * m_map->width() / kTileSize +
                             pos["x_pixel"].as<int>() / kTileSize;
                m_all_land_tiles[index] = true;
            }
        }

        for (const auto& pos : node["point_to_gps_position"])
        {
            if (pos["x_pixel"] && pos["y_pixel"] && pos["longitude"] && pos["latitude"])
            {
                auto x = pos["x_pixel"].as<int>();
                auto y = pos["y_pixel"].as<int>();

                if (x >= m_map->width() || y >= m_map->height())
                {
                    continue;
                }

                auto index = (y / kGpsPositionSize) * (m_map->width() / kGpsPositionSize) +
                             x / kGpsPositionSize;
                m_gps_positions[index] = {.latitude = pos["latitude"].as<float>(),
                                          .longitude = pos["longitude"].as<float>()};
            }
        }

        if (node["home_position"])
        {
            auto home_pos = node["home_position"];
            if (home_pos["x_pixel"] && home_pos["y_pixel"])
            {
                m_home_position =
                    Point {home_pos["x_pixel"].as<int>(), home_pos["y_pixel"].as<int>()};
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

    if (!m_skip_tiles.empty())
    {
        YAML::Node skip_tiles;
        for (const auto& pos : m_skip_tiles)
        {
            YAML::Node pos_node;

            pos_node["x_pixel"] = pos.first;
            pos_node["y_pixel"] = pos.second;

            skip_tiles.push_back(pos_node);
        }

        node["skip_tiles"] = skip_tiles;
    }

    if (m_home_position)
    {
        YAML::Node home_position;

        home_position["x_pixel"] = m_home_position->x;
        home_position["y_pixel"] = m_home_position->y;

        node["home_position"] = home_position;
    }

    YAML::Node gps_positions;
    for (auto i = 0; i < m_gps_positions.size(); ++i)
    {
        auto y = i / (m_map->width() / kTileSize);
        auto x = i % (m_map->width() / kTileSize);

        YAML::Node pos_node;

        pos_node["x_pixel"] = x * kTileSize;
        pos_node["y_pixel"] = y * kTileSize;
        pos_node["longitude"] = m_gps_positions[i].longitude;
        pos_node["latitude"] = m_gps_positions[i].latitude;
        pos_node["longitude_offset"] = m_gps_positions[i].longitude_offset;
        pos_node["latitude_offset"] = m_gps_positions[i].latitude_offset;

        gps_positions.push_back(pos_node);
    }
    node["point_to_gps_position"] = gps_positions;


    YAML::Node all_land_tiles;
    for (auto y = 0; y < m_map->height(); y += kTileSize)
    {
        for (auto x = 0; x < m_map->width(); x += kTileSize)
        {
            if (m_all_land_tiles[(y / kTileSize) * m_map->width() / kTileSize + x / kTileSize])
            {
                YAML::Node pos_node;

                pos_node["x_pixel"] = x;
                pos_node["y_pixel"] = y;

                all_land_tiles.push_back(pos_node);
            }
        }
    }

    node["all_land_tiles"] = all_land_tiles;

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

    for (auto y = 0; y < m_map->height(); y += kTileSize)
    {
        for (auto x = 0; x < m_map->width(); x += kTileSize)
        {
            bool water = false;

            for (auto y2 = y; y2 < y + kTileSize; y2 += kPathFinderTileSize)
            {
                for (auto x2 = x; x2 < x + kTileSize; x2 += kPathFinderTileSize)
                {
                    if (m_land_mask[(y2 / kPathFinderTileSize) * m_map->width() /
                                        kPathFinderTileSize +
                                    x2 / kPathFinderTileSize] == false)
                    {
                        water = true;
                        break;
                    }
                }
                if (water)
                {
                    break;
                }
            }

            if (!water)
            {
                m_all_land_tiles[(y / kTileSize) * m_map->width() / kTileSize + x / kTileSize] =
                    true;
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

void
MapEditorMainWindow::AddSkipTile(int x, int y)
{
    x = x - x % kTileSize;
    y = y - y % kTileSize;

    m_skip_tiles.insert({x, y});
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
