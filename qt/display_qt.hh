#pragma once

#include "hal/i_display.hh"

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
    void AlphaBlit(const Image& image,
                   uint8_t alpha_percent,
                   Rect to,
                   std::optional<Rect> from = std::nullopt) final;
    void DrawAlphaLine(Point from, Point to, uint8_t width,  uint16_t rgb565,uint8_t alpha_byte) final;
    void Flip() final;

signals:
    void DoFlip();

private slots:
    void UpdateScreen();


private:
    std::unique_ptr<QImage> m_screen;
    QImage m_circle_mask;
    QGraphicsPixmapItem* m_pixmap;

    std::array<uint16_t, hal::kDisplayWidth * hal::kDisplayHeight> m_frame_buffer;
};
