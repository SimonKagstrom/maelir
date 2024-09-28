#pragma once

#include "image.hh"
#include "rect.hh"
#include "tile.hh"

#include <optional>

namespace painter
{

void
Blit(uint16_t* frame_buffer, const Image& image, Rect to, std::optional<Rect> from = std::nullopt);

void AlphaBlit(uint16_t* frame_buffer,
               const Image& image,
               uint8_t alpha_byte,
               Rect to,
               std::optional<Rect> from = std::nullopt);

void DrawAlphaLine(uint16_t* frame_buffer,
                   Point from,
                   Point to,
                   uint8_t width,
                   uint16_t rgb565,
                   uint8_t alpha_byte);

Image Rotate(const Image &src, std::span<uint16_t> dst, int angle);

} // namespace painter
