#include "mapeditor_graphicsview.hh"

#include "mapeditor_mainwindow.hh"

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
MapEditorGraphicsView::drawForeground(QPainter* painter, const QRectF& rect)
{
    Q_UNUSED(rect);

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

    for (const auto& cur : m_owner->m_current_route)
    {
        QPen pen(Qt::yellow);
        pen.setWidth(3);

        painter->setPen(pen);
        painter->drawRect(cur.x, cur.y, kPathFinderTileSize, kPathFinderTileSize);
    }

    // Get the visible area in scene coordinates
    QRectF visibleArea = mapToScene(viewport()->rect()).boundingRect();

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
                // Set brush to NoBrush to make the rectangles hollow
                painter->setBrush(Qt::NoBrush);
                QPen pen(Qt::red);
                pen.setWidth(1); // Set the pen width to 3 pixels
                painter->setPen(pen);

                painter->drawRect(tileRect);
            }
        }
    }
}