#include "display_qt.hh"

DisplayQt::DisplayQt(QGraphicsScene* scene)
    : m_screen(
          std::make_unique<QImage>(hal::kDisplayWidth, hal::kDisplayHeight, QImage::Format_RGB32))
    , m_pixmap(scene->addPixmap(QPixmap::fromImage(*m_screen)))
{
    connect(this, SIGNAL(DoFlip()), this, SLOT(UpdateScreen()));
}


void
DisplayQt::Blit(const Image& image, Rect from, Rect to)
{
    for (int y = 0; y < from.height; ++y)
    {
        for (int x = 0; x < from.width; ++x)
        {
            auto dst_x = to.x + x;
            auto dst_y = to.y + y;

            if (dst_x >= hal::kDisplayWidth || dst_y >= hal::kDisplayHeight)
            {
                continue;
            }

            auto rgb565 = __builtin_bswap16(image.data[(from.y + y) * image.width + from.x + x]);
            auto r = (rgb565 >> 11) & 0x1F;
            auto g = (rgb565 >> 5) & 0x3F;
            auto b = rgb565 & 0x1F;

            r = (r * 255) / 31;
            g = (g * 255) / 63;
            b = (b * 255) / 31;

            auto color = QColor(r, g, b);

            m_screen->setPixelColor(dst_x, dst_y, color);
        }
    }
}

void
DisplayQt::Flip()
{
    emit DoFlip();
}

void
DisplayQt::UpdateScreen()
{
    m_pixmap->setPixmap(QPixmap::fromImage(*m_screen));
}
