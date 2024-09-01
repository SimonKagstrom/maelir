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
}
