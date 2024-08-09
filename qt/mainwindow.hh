#pragma once

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QImage>
#include <QMainWindow>

#include "display_qt.hh"

namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() final;

private slots:

private:
    Ui::MainWindow* m_ui {nullptr};

    std::unique_ptr<QGraphicsScene> m_scene;
    std::unique_ptr<DisplayQt> m_display;
};
