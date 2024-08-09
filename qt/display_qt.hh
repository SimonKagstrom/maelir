#pragma once

#include "i_display.hh"

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QImage>
#include <QObject>

class DisplayQt : public QObject, public hal::IDisplay
{
    Q_OBJECT

public:
    DisplayQt(QGraphicsScene* scene);

    void Blit(const Image& image, Rect from, Rect to) final;
    void Flip() final;

signals:
    void DoFlip();

private slots:
    void UpdateScreen();


private:
    std::unique_ptr<QImage> m_screen;
    QGraphicsPixmapItem* m_pixmap;
};
