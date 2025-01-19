
#include "painter.hh"

#include "hal/i_display.hh"

#include <cmath>
#include <cstring>

namespace
{

auto
Prepare(const auto& image, auto& to)
{
    int32_t height = image.Height();
    int32_t width = image.Width();
    int32_t from_y = 0;
    int32_t from_x = 0;

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

    int32_t row_length = image.Width() - from_x;

    to.x = std::max(static_cast<int32_t>(0), to.x);
    to.y = std::max(static_cast<int32_t>(0), to.y);
    if (to.x + row_length > hal::kDisplayWidth)
    {
        row_length = hal::kDisplayWidth - to.x;
    }
    if (row_length > width)
    {
        row_length = width;
    }

    return std::array {height, width, from_y, from_x, row_length};
}

} // namespace

namespace painter
{

void
Blit(uint16_t* frame_buffer, const Image& image, Rect to)
{
    auto [height, width, from_y, from_x, row_length] = Prepare(image, to);

    for (int y = 0; y < height; ++y)
    {
        auto dst_y = to.y + y;

        if (dst_y < 0 || dst_y >= hal::kDisplayHeight)
        {
            continue;
        }

        memcpy(&frame_buffer[dst_y * hal::kDisplayWidth + to.x],
               &image.Data16()[(from_y + y) * image.Width() + from_x],
               row_length * sizeof(uint16_t));
    }
}

void
ZoomedBlit(uint16_t* frame_buffer, uint32_t buffer_width, const Image& image, unsigned factor, Rect to)
{
    auto [height, width, from_y, from_x, row_length] = Prepare(image, to);

    for (auto y = 0; y < height; y += factor)
    {
        auto dst_y = to.y + y / factor;

        if (dst_y < 0 || dst_y >= hal::kDisplayHeight)
        {
            continue;
        }

        for (auto x = 0; x < row_length * factor; x += factor)
        {
            auto dst_x = to.x + x / factor;
            auto src_x = from_x + x;
            auto src_y = from_y + y;

            if (dst_x < 0 || dst_x >= buffer_width)
            {
                continue;
            }
            if (src_x < 0 || src_x >= image.Width() || src_y < 0 || src_y >= image.Height())
            {
                continue;
            }

            auto src_color = image.Data16()[src_y * image.Width() + src_x];
            frame_buffer[dst_y * buffer_width + dst_x] = src_color;
        }
    }
}

} // namespace painter
