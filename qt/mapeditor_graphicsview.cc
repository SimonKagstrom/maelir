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

    if (m_owner->m_show_map_tiles)
    {
        drawTileRects(painter, visibleArea);
    }
    if (m_owner->m_show_gps_tiles)
    {
        drawGpsTileRects(painter, visibleArea);
    }

    if (!m_owner->m_current_route.empty())
    {
        const auto row_size = m_owner->m_map->width() / kPathFinderTileSize;

        auto route_iterator = RouteIterator(m_owner->m_current_route, row_size);

        auto last = route_iterator.Next();
        while (auto cur = route_iterator.Next())
        {
            QPen pen(Qt::yellow);
            pen.setWidth(3);

            painter->setPen(pen);
            // Will draw twice, but let's ignore that for now
            painter->drawRect(last->x, last->y, kPathFinderTileSize, kPathFinderTileSize);
            painter->drawRect(cur->x, cur->y, kPathFinderTileSize, kPathFinderTileSize);

            // And a line between them
            painter->drawLine(last->x + kPathFinderTileSize / 2,
                              last->y + kPathFinderTileSize / 2,
                              cur->x + kPathFinderTileSize / 2,
                              cur->y + kPathFinderTileSize / 2);

            last = cur;
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

void
MapEditorGraphicsView::drawTileRects(QPainter* painter, const QRectF& visibleArea)
{
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
                QPen pen(Qt::darkCyan);
                pen.setDashPattern({2, 2});
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
}


void
MapEditorGraphicsView::drawGpsTileRects(QPainter* painter, const QRectF& visibleArea)
{
    // Fill the 256x256 tiles with black rectangles
    for (int y = 0; y < m_owner->m_map->height(); y += kGpsPositionSize)
    {
        for (int x = 0; x < m_owner->m_map->width(); x += kGpsPositionSize)
        {
            QRectF tileRect(x, y, kGpsPositionSize, kGpsPositionSize);
            if (visibleArea.intersects(tileRect))
            {
                auto index = (y / kGpsPositionSize) * m_owner->m_gps_map_width / kGpsPositionSize +
                             x / kGpsPositionSize;
                const auto& position = m_owner->m_gps_positions[index];

                auto l1 = std::format("{:.4f}, {:.4f} -> {:.4f}, {:.4f}\nx,y: {},{}",
                                      position.latitude,
                                      position.longitude,
                                      position.latitude + position.latitude_offset,
                                      position.longitude + position.longitude_offset,
                                      x,
                                      y);

                QString text = QString("%1").arg(l1.c_str());
                painter->drawText(QRectF(x + 2, y + 2, kGpsPositionSize, kGpsPositionSize),
                                  QTextOption::WordWrap,
                                  text);


                // Set brush to NoBrush to make the rectangles hollow
                painter->setBrush(Qt::NoBrush);
                QPen pen(Qt::black);
                pen.setWidth(2);
                pen.setStyle(Qt::DashLine); // Set the pen style to dashed
                painter->setPen(pen);

                painter->drawRect(tileRect);
            }
        }
    }
}
