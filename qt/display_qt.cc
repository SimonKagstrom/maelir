#include "display_qt.hh"

#include <QPainter>

DisplayQt::DisplayQt(QGraphicsScene* scene)
    : m_screen(
          std::make_unique<QImage>(hal::kDisplayWidth, hal::kDisplayHeight, QImage::Format_RGB32))
    , m_pixmap(scene->addPixmap(QPixmap::fromImage(*m_screen)))
{
    connect(this, SIGNAL(DoFlip()), this, SLOT(UpdateScreen()));
}


void
DisplayQt::Blit(const Image& image, Rect to, std::optional<Rect> from)
{
    auto height = from.has_value() ? from->height : image.height;
    auto width = from.has_value() ? from->width : image.width;
    auto from_y = from.has_value() ? from->y : 0;
    auto from_x = from.has_value() ? from->x : 0;


    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            auto dst_x = to.x + x;
            auto dst_y = to.y + y;

            if (dst_x >= hal::kDisplayWidth || dst_y >= hal::kDisplayHeight)
            {
                continue;
            }

            auto rgb565 = image.data[(from_y + y) * image.width + from_x + x];
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
    // (from copilot)
    QPixmap pixmap = QPixmap::fromImage(*m_screen);

    // Create a QPainter to draw on the pixmap
    QPainter painter(&pixmap);

    // Set the pen and brush for drawing
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::black);

    // Calculate the center and radius of the circle
    int width = pixmap.width();
    int height = pixmap.height();

    // Create a mask region
    QRegion maskRegion(0, 0, width, height);
    QRegion circleRegion(0, 0, width, height, QRegion::Ellipse);
    maskRegion = maskRegion.subtracted(circleRegion);

    // Fill the masked area with black
    painter.setClipRegion(maskRegion);
    painter.fillRect(0, 0, width, height, Qt::black);

    // Set the modified pixmap to the label
    m_pixmap->setPixmap(pixmap);
}
