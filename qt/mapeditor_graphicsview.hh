#pragma once

#include <QGraphicsView>
#include <QPointF>
#include <QVector>

class MapEditorGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit MapEditorGraphicsView(QWidget* parent = nullptr);

    void SetGpsPositions(const QVector<QPointF>& positions);

protected:
    void drawForeground(QPainter* painter, const QRectF& rect) override;

private:
    QVector<QPointF> m_positions;
};
