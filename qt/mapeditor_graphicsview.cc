#include "mapeditor_graphicsview.hh"

#include <QPainter>

MapEditorGraphicsView::MapEditorGraphicsView(QWidget* parent)
    : QGraphicsView(parent)
{
}

void
MapEditorGraphicsView::SetGpsPositions(const QVector<QPointF>& positions)
{
    m_positions = positions;
    viewport()->update(); // Trigger a repaint
}

void
MapEditorGraphicsView::drawForeground(QPainter* painter, const QRectF& rect)
{
    Q_UNUSED(rect);

    painter->setPen(Qt::NoPen);
    painter->setBrush(Qt::magenta);

    // Draw a dot for each GPS position
    for (const auto& pos : m_positions)
    {
        painter->drawEllipse(pos, 6, 6);
    }
}