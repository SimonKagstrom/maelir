#include "mapeditor_graphicsview.hh"

#include "mapeditor_mainwindow.hh"
#include "route_iterator.hh"
#include "route_utils.hh"

#include <QPainter>

MapEditorGraphicsView::MapEditorGraphicsView(QWidget* parent)
    : QGraphicsView(parent)
{
}

void
MapEditorGraphicsView::SetOwner(MapEditorMainWindow* owner)
{
    m_owner = owner;
}

void
MapEditorGraphicsView::drawForeground(QPainter* painter, const QRectF&)
{
    // Get the visible area in scene coordinates
    QRectF visibleArea = mapToScene(viewport()->rect()).boundingRect();

    painter->setPen(Qt::NoPen);

    QVector<QPointF> scene_positions;
    for (auto pos : m_owner->m_positions)
    {
        scene_positions.push_back(QPointF(pos.x, pos.y));
    }

    // Draw a dot for each GPS position
    for (const auto& pos : scene_positions)
    {
        painter->setBrush(Qt::magenta);
        painter->drawEllipse(pos, 8, 8);
        painter->setBrush(Qt::cyan);
        painter->drawEllipse(pos, 5, 5);
    }

    // Draw a dot for each GPS position
    for (const auto& cur : m_owner->m_wanted_route)
    {
        auto inner = QRect(cur.x, cur.y, 5, 5);
        auto outer = QRect(cur.x, cur.y, 8, 8);

        painter->setBrush(Qt::magenta);
        painter->drawRect(inner);
        painter->setBrush(Qt::cyan);
        painter->drawRect(outer);
    }

    for (const auto [x, y] : m_owner->m_skip_tiles)
    {
        QRectF tileRect(x, y, kTileSize, kTileSize);
        if (visibleArea.intersects(tileRect))
        {
            // Convert the underlying map image to grayscale
            QImage grayscaleImage =
                m_owner->m_map->copy(tileRect.toRect()).convertToFormat(QImage::Format_Grayscale8);

            // Draw the grayscale image
            painter->drawImage(tileRect, grayscaleImage);
        }
    }

    if (!m_owner->m_current_route.empty())
    {
        const auto row_size = m_owner->m_map->width() / kPathFinderTileSize;

        auto route_iterator =
            RouteIterator(m_owner->m_current_route,
                          PointToLandIndex({0, 0}, row_size),
                          PointToLandIndex({m_owner->m_map->width() / kPathFinderTileSize,
                                            m_owner->m_map->height() / kPathFinderTileSize},
                                           row_size),
                          row_size);

        auto last = route_iterator.Next();
        while (auto cur = route_iterator.Next())
        {
            auto index = *cur;
            auto last_x = *last % row_size;
            auto last_y = *last / row_size;
            auto cur_x = index % row_size;
            auto cur_y = index / row_size;

            QPen pen(Qt::yellow);
            pen.setWidth(3);

            painter->setPen(pen);
            // Will draw twice, but let's ignore that for now
            painter->drawRect(last_x * kPathFinderTileSize,
                              last_y * kPathFinderTileSize,
                              kPathFinderTileSize,
                              kPathFinderTileSize);
            painter->drawRect(cur_x * kPathFinderTileSize,
                              cur_y * kPathFinderTileSize,
                              kPathFinderTileSize,
                              kPathFinderTileSize);
            // And a line between them
            painter->drawLine(last_x * kPathFinderTileSize + kPathFinderTileSize / 2,
                              last_y * kPathFinderTileSize + kPathFinderTileSize / 2,
                              cur_x * kPathFinderTileSize + kPathFinderTileSize / 2,
                              cur_y * kPathFinderTileSize + kPathFinderTileSize / 2);

            last = cur;
        }
    }

    // Fill the 240x240 tiles with black rectangles
    for (int y = 0; y < m_owner->m_map->height(); y += kTileSize)
    {
        for (int x = 0; x < m_owner->m_map->width(); x += kTileSize)
        {
            QRectF tileRect(x, y, kTileSize, kTileSize);
            if (visibleArea.intersects(tileRect))
            {
                // Set brush to NoBrush to make the rectangles hollow
                painter->setBrush(Qt::NoBrush);
                QPen pen(Qt::black);
                pen.setWidth(2);
                painter->setPen(pen);

                painter->drawRect(tileRect);

                if (m_owner
                        ->m_all_land_tiles[(y / kTileSize) * m_owner->m_map->width() / kTileSize +
                                           x / kTileSize])
                {

                    painter->drawRect(tileRect);
                    // Convert the underlying map image to grayscale
                    QImage grayscaleImage = m_owner->m_map->copy(tileRect.toRect())
                                                .convertToFormat(QImage::Format_Grayscale8);

                    // Draw the grayscale image
                    painter->drawImage(tileRect, grayscaleImage);
                }
            }
        }
    }

    // Fill the visible land with red boxes
    for (int y = 0; y < m_owner->m_map->height(); y += kPathFinderTileSize)
    {
        for (int x = 0; x < m_owner->m_map->width(); x += kPathFinderTileSize)
        {
            QRectF tileRect(x, y, kPathFinderTileSize, kPathFinderTileSize);
            if (visibleArea.intersects(tileRect) &&
                m_owner->m_land_mask[(y / kPathFinderTileSize) * m_owner->m_map->width() /
                                         kPathFinderTileSize +
                                     x / kPathFinderTileSize])
            {
                painter->setBrush(Qt::NoBrush);
                QPen pen(Qt::red);
                pen.setWidth(1);
                painter->setPen(pen);

                painter->drawRect(tileRect);
            }
        }
    }
}
