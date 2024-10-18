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


    connect(m_ui->leftButton, &QPushButton::clicked, [this]() {
        Event event;
        event.button = 0;
        event.type = EventType::kLeft;

        m_input_listener->OnInput(event);
    });
    connect(m_ui->rightButton, &QPushButton::clicked, [this]() {
        Event event;
        event.button = 0;
        event.type = EventType::kRight;

        m_input_listener->OnInput(event);
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
