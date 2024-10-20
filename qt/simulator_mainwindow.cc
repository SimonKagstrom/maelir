#include "simulator_mainwindow.hh"

#include "ui_simulator_mainwindow.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_ui(new Ui::MainWindow)
{
    m_ui->setupUi(this);

    m_scene = std::make_unique<QGraphicsScene>();
    m_display = std::make_unique<DisplayQt>(m_scene.get());
    m_ui->displayGraphicsView->setScene(m_scene.get());


    connect(m_ui->leftButton, &QPushButton::clicked, [this]() { ButtonEvent(EventType::kLeft); });
    connect(m_ui->rightButton, &QPushButton::clicked, [this]() { ButtonEvent(EventType::kRight); });
    connect(m_ui->centerButton, &QPushButton::pressed, [this]() {
        ButtonEvent(EventType::kButtonDown);
    });
    connect(m_ui->centerButton, &QPushButton::released, [this]() {
        ButtonEvent(EventType::kButtonUp);
    });
}

MainWindow::~MainWindow()
{
    delete m_ui;
}

hal::IDisplay&
MainWindow::GetDisplay()
{
    return *m_display;
}

void
MainWindow::AttachListener(hal::IInput::IListener* listener)
{
    m_input_listener = listener;
}

void
MainWindow::ButtonEvent(hal::IInput::EventType type)
{
    Event event;
    event.button = 0;
    event.type = type;

    assert(m_input_listener);
    m_input_listener->OnInput(event);
}
