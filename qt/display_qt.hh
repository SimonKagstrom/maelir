#pragma once

#include "i_display.hh"

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QImage>

class DisplayQt : public hal::IDisplay
{
public:
    DisplayQt(QGraphicsScene *scene);

    void Blit(const Image& image, Rect from, Rect to) final;
    void Flip() final;

private:
    std::unique_ptr<QImage> m_screen;
    QGraphicsPixmapItem* m_pixmap;
};
