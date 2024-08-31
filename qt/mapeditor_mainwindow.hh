#pragma once

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QImage>
#include <QMainWindow>
#include <QMouseEvent>

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
    bool filterMouse(QObject* obj, QEvent* event);

private slots:

private:
    Ui::MainWindow* m_ui {nullptr};

    std::unique_ptr<QGraphicsScene> m_scene;
    std::unique_ptr<QImage> m_map;
    QGraphicsPixmapItem* m_pixmap;

    bool m_panning;
    QPoint m_lastMousePos;
};
