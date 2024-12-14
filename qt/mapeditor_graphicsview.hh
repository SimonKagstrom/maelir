#pragma once

#include <QGraphicsView>
#include <QPointF>
#include <QVector>

class MapEditorMainWindow;

class MapEditorGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit MapEditorGraphicsView(QWidget* parent = nullptr);

    void SetOwner(MapEditorMainWindow* owner);

protected:
    void drawForeground(QPainter* painter, const QRectF& rect) override;
    void drawTileRects(QPainter* painter, const QRectF& rect);
    void drawGpsTileRects(QPainter* painter, const QRectF& rect);

private:
    MapEditorMainWindow* m_owner {nullptr};
};
