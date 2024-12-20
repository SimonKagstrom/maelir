
#include "painter.hh"

#include "hal/i_display.hh"

#include <cmath>
#include <cstring>

namespace
{

auto
Prepare(const auto& image, auto& from, auto& to)
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
Blit(uint16_t* frame_buffer, const Image& image, Rect to, std::optional<Rect> from)
{
    auto [height, width, from_y, from_x, row_length] = Prepare(image, from, to);

    for (int y = 0; y < height; ++y)
    {
        auto dst_y = to.y + y;

        if (dst_y < 0 || dst_y >= hal::kDisplayHeight)
        {
            continue;
        }

        memcpy(&frame_buffer[dst_y * hal::kDisplayWidth + to.x],
               &image.data[(from_y + y) * image.width + from_x],
               row_length * sizeof(uint16_t));
    }
}

void
ZoomedBlit(
    uint16_t* frame_buffer, const Image& image, unsigned factor, Rect to, std::optional<Rect> from)
{
    auto [height, width, from_y, from_x, row_length] = Prepare(image, from, to);

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

            if (dst_x < 0 || dst_x >= hal::kDisplayWidth)
            {
                continue;
            }
            if (src_x < 0 || src_x >= image.width || src_y < 0 || src_y >= image.height)
            {
                continue;
            }

            auto src_color = image.data[src_y * image.width + src_x];
            frame_buffer[dst_y * hal::kDisplayWidth + dst_x] = src_color;
        }
    }
}

void
MaskBlit(uint16_t* frame_buffer, const Image& image, Rect to, std::optional<Rect> from)
{
    auto [height, width, from_y, from_x, row_length] = Prepare(image, from, to);

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
            if (src_color == 0)
            {
                continue;
            }

            frame_buffer[dst_y * hal::kDisplayWidth + dst_x] = src_color;
        }
    }
}


void
AlphaBlit(uint16_t* frame_buffer,
          const Image& image,
          uint8_t alpha_byte,
          Rect to,
          std::optional<Rect> from)
{
    auto [height, width, from_y, from_x, row_length] = Prepare(image, from, to);

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
            auto dst_color = frame_buffer[dst_y * hal::kDisplayWidth + dst_x];

            auto src_r = (src_color >> 11) & 0x1F;
            auto src_g = (src_color >> 5) & 0x3F;
            auto src_b = src_color & 0x1F;

            auto dst_r = (dst_color >> 11) & 0x1F;
            auto dst_g = (dst_color >> 5) & 0x3F;
            auto dst_b = dst_color & 0x1F;

            // Ignore black
            if (src_r == 0 && src_g == 0 && src_b == 0)
            {
                continue;
            }

            auto r = ((src_r * alpha_byte) + (dst_r * (255 - alpha_byte))) / 255;
            auto g = ((src_g * alpha_byte) + (dst_g * (255 - alpha_byte))) / 255;
            auto b = ((src_b * alpha_byte) + (dst_b * (255 - alpha_byte))) / 255;

            frame_buffer[dst_y * hal::kDisplayWidth + dst_x] = (r << 11) | (g << 5) | b;
        }
    }
}

void
DrawAlphaLine(uint16_t* frame_buffer,
              Point from,
              Point to,
              uint8_t width,
              uint16_t rgb565,
              uint8_t alpha_byte)
{
    // Only draws horizontal, vertical, or diagonal lines
    auto direction = PointPairToVector(from, to);
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
                auto src_color = frame_buffer[cur.y * hal::kDisplayWidth + cur.x];

                auto src_r = (src_color >> 11) & 0x1F;
                auto src_g = (src_color >> 5) & 0x3F;
                auto src_b = src_color & 0x1F;

                auto r = ((src_r * alpha_byte) + (dst_r * (255 - alpha_byte))) / 255;
                auto g = ((src_g * alpha_byte) + (dst_g * (255 - alpha_byte))) / 255;
                auto b = ((src_b * alpha_byte) + (dst_b * (255 - alpha_byte))) / 255;

                frame_buffer[cur.y * hal::kDisplayWidth + cur.x + x] = (r << 11) | (g << 5) | b;
            }
        }

        next = next + direction;
    }
}

void
DrawNumbers(uint16_t* frame_buffer, const Image& image, Point to, std::string_view string)
{
    auto number_width = image.width / 10;

    for (auto c : string)
    {
        auto number = c - '0';
        auto from = Rect {number * number_width, 0, number_width, image.height};
        auto to_rect = Rect {to.x, to.y, number_width, image.height};

        MaskBlit(frame_buffer, image, to_rect, from);
        to.x += number_width;
    }
}


// From https://github.com/anim1311/Image-Rotation
Image
Rotate(const Image& image, std::span<uint16_t> new_image_data, int angle)
{
    // Convert angle to radians
    float radians = angle * M_PI / 180.0f;

    // Compute sine and cosine of angle
    float s = sin(radians);
    float c = cos(radians);

    // Compute new image size and allocate memory for it
    int new_width = (int)round(abs(image.width * c) + abs(image.height * s));
    int new_height = (int)round(abs(image.width * s) + abs(image.height * c));

    // Initialize new image with black
    memset(new_image_data.data(), 0, new_width * new_height * sizeof(uint16_t));

    // Compute center of old and new images
    auto old_cx = (float)image.width / 2.0f;
    auto old_cy = (float)image.height / 2.0f;
    auto new_cx = (float)new_width / 2.0f;
    auto new_cy = (float)new_height / 2.0f;

    // Iterate over new image pixels and map them back to old image pixels
    for (int y = 0; y < new_height; y++)
    {
        for (int x = 0; x < new_width; x++)
        {
            // Compute old image pixel coordinates corresponding to current new image pixel
            float old_x = (x - new_cx) * c + (y - new_cy) * s + old_cx;
            float old_y = -(x - new_cx) * s + (y - new_cy) * c + old_cy;

            // Check if old image pixel coordinates are within bounds
            if (old_x >= 0 && old_x < image.width && old_y >= 0 && old_y < image.height)
            {
                // Compute indices of old image pixel
                int old_index = ((int)old_x + (int)old_y * image.width);

                // Copy old image pixel to new image
                int new_index = (x + y * new_width);

                // Copy old image pixel to new image
                new_image_data[new_index] = image.data[old_index];
            }
        }
    }

    return Image(new_image_data, new_width, new_height);
}

std::vector<uint16_t>
AllocateRotationBuffer(const Image& src)
{
    auto size = static_cast<size_t>(src.width * 1.5f * src.height * 1.5f);
    auto out = std::vector<uint16_t>(size);

    return out;
}

} // namespace painter
