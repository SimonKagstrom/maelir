#pragma once

#include "display_qt.hh"
#include "hal/i_input.hh"

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QImage>
#include <QMainWindow>

namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow, public hal::IInput
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() final;

    hal::IDisplay& GetDisplay();

private slots:


private:
    void AttachListener(hal::IInput::IListener* listener) final;

    void ButtonEvent(hal::IInput::EventType type);

    Ui::MainWindow* m_ui {nullptr};

    std::unique_ptr<QGraphicsScene> m_scene;
    std::unique_ptr<DisplayQt> m_display;
    hal::IInput::IListener* m_input_listener {nullptr};
};
