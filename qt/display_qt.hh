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

    void Blit(const Image& image, Rect to, std::optional<Rect> from = std::nullopt) final;
    void Flip() final;

signals:
    void DoFlip();

private slots:
    void UpdateScreen();


private:
    std::unique_ptr<QImage> m_screen;
    QImage m_circle_mask;
    QGraphicsPixmapItem* m_pixmap;
};
