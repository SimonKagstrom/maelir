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

    if (to.x < 0)
    {
        from_x += -to.x;
        width += to.x;
    }
    if (to.y < 0)
    {
        from_y += -to.y;
        height += to.y;
    }

    auto row_length = image.width - from_x;

    to.x = std::max(0, to.x);
    to.y = std::max(0, to.y);
    if (to.x + row_length > hal::kDisplayWidth)
    {
        row_length = hal::kDisplayWidth - to.x;
    }

    for (int y = 0; y < height; ++y)
    {
        auto dst_y = to.y + y;

        if (dst_y < 0 || dst_y >= hal::kDisplayHeight)
        {
            continue;
        }

        memcpy(&m_frame_buffer[dst_y * hal::kDisplayWidth + to.x],
               &image.data[(from_y + y) * image.width + from_x],
               row_length * sizeof(uint16_t));
    }
}

void
DisplayQt::AlphaBlit(const Image& image, uint8_t alpha_byte, Rect to, std::optional<Rect> from)
{
    auto height = from.has_value() ? from->height : image.height;
    auto width = from.has_value() ? from->width : image.width;
    auto from_y = from.has_value() ? from->y : 0;
    auto from_x = from.has_value() ? from->x : 0;

    if (to.x < 0)
    {
        from_x += -to.x;
        width += to.x;
    }
    if (to.y < 0)
    {
        from_y += -to.y;
        height += to.y;
    }

    auto row_length = image.width - from_x;

    to.x = std::max(0, to.x);
    to.y = std::max(0, to.y);
    if (to.x + row_length > hal::kDisplayWidth)
    {
        row_length = hal::kDisplayWidth - to.x;
    }

    for (int y = 0; y < height; ++y)
    {
        auto dst_y = to.y + y;

        if (dst_y < 0 || dst_y >= hal::kDisplayHeight)
        {
            continue;
        }

        for (auto x = 0; x < row_length; ++x)
        {
            auto dst_x = to.x + x;
            auto src_x = from_x + x;
            auto src_y = from_y + y;

            if (dst_x < 0 || dst_x >= hal::kDisplayWidth)
            {
                continue;
            }

            auto src_color = image.data[src_y * image.width + src_x];
            auto dst_color = m_frame_buffer[dst_y * hal::kDisplayWidth + dst_x];

            auto src_r = (src_color >> 11) & 0x1F;
            auto src_g = (src_color >> 5) & 0x3F;
            auto src_b = src_color & 0x1F;

            auto dst_r = (dst_color >> 11) & 0x1F;
            auto dst_g = (dst_color >> 5) & 0x3F;
            auto dst_b = dst_color & 0x1F;

            auto r = ((src_r * alpha_byte) + (dst_r * (255 - alpha_byte))) / 255;
            auto g = ((src_g * alpha_byte) + (dst_g * (255 - alpha_byte))) / 255;
            auto b = ((src_b * alpha_byte) + (dst_b * (255 - alpha_byte))) / 255;

            m_frame_buffer[dst_y * hal::kDisplayWidth + dst_x] = (r << 11) | (g << 5) | b;
        }
    }
}

void
DisplayQt::DrawAlphaLine(Point from, Point to, uint8_t width, uint16_t rgb565, uint8_t alpha_byte)
{
    // Only draws horizontal, vertical, or diagonal lines
    auto direction = PointPairToDirection(from, to);
    auto perpendicular = direction.Perpendicular();
    auto next = from;

    auto dst_r = (rgb565 >> 11) & 0x1F;
    auto dst_g = (rgb565 >> 5) & 0x3F;
    auto dst_b = rgb565 & 0x1F;

    auto pixel_w = perpendicular.IsDiagonal() ? 2 : 1;
    if (perpendicular.IsDiagonal())
    {
        // Looks better
        width = std::max(width - 1, 1);
    }

    while (next != to)
    {
        for (auto w = -width / 2; w < width / 2; ++w)
        {
            auto cur = next + perpendicular * w;

            if (cur.y < 0 || cur.y >= hal::kDisplayHeight)
            {
                continue;
            }
            for (auto x = 0; x < pixel_w; x++)
            {
                if (cur.x + x < 0 || cur.x + x >= hal::kDisplayWidth)
                {
                    continue;
                }
                auto src_color = m_frame_buffer[cur.y * hal::kDisplayWidth + cur.x];

                auto src_r = (src_color >> 11) & 0x1F;
                auto src_g = (src_color >> 5) & 0x3F;
                auto src_b = src_color & 0x1F;

                auto r = ((src_r * alpha_byte) + (dst_r * (255 - alpha_byte))) / 255;
                auto g = ((src_g * alpha_byte) + (dst_g * (255 - alpha_byte))) / 255;
                auto b = ((src_b * alpha_byte) + (dst_b * (255 - alpha_byte))) / 255;

                m_frame_buffer[cur.y * hal::kDisplayWidth + cur.x + x] = (r << 11) | (g << 5) | b;
            }
        }

        next = next + direction;
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
    for (int y = 0; y < hal::kDisplayHeight; ++y)
    {
        for (int x = 0; x < hal::kDisplayWidth; ++x)
        {
            auto rgb565 = m_frame_buffer[y * hal::kDisplayWidth + x];
            auto r = (rgb565 >> 11) & 0x1F;
            auto g = (rgb565 >> 5) & 0x3F;
            auto b = rgb565 & 0x1F;

            r = (r * 255) / 31;
            g = (g * 255) / 63;
            b = (b * 255) / 31;

            auto color = QColor(r, g, b);

            m_screen->setPixelColor(x, y, color);
        }
    }


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
