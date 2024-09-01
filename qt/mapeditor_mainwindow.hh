#pragma once

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QImage>
#include <QMainWindow>
#include <QMouseEvent>
#include <etl/list.h>

namespace Ui
{
class MainWindow;
}

class MapEditorMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MapEditorMainWindow(std::unique_ptr<QImage> map, QWidget* parent = nullptr);
    ~MapEditorMainWindow() final;

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:

private:
    struct GpsToPixel
    {
        double longitude;
        double latitude;
        int x;
        int y;
    };

    bool FilterMouse(QObject* obj, QEvent* event);
    std::pair<int, int> GetMapCoordinates(QPoint pos);
    void RightClickContextMenu(QPoint mouse_position, QPoint map_posititon);
    void SetGpsPosition(double longitude, double latitude, int x, int y);


    Ui::MainWindow* m_ui {nullptr};

    std::unique_ptr<QGraphicsScene> m_scene;
    std::unique_ptr<QImage> m_map;
    QGraphicsPixmapItem* m_pixmap;

    etl::list<GpsToPixel, 2> m_positions;

    bool m_panning {false};
    QPoint m_last_mouse_pos {0, 0};
};
