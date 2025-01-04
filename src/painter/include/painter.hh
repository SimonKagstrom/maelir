#pragma once

#include "image.hh"
#include "rect.hh"
#include "tile.hh"

#include <optional>
#include <string_view>
#include <vector>

namespace painter
{

void
Blit(uint16_t* frame_buffer, const Image& image, Rect to, std::optional<Rect> from = std::nullopt);

void ZoomedBlit(uint16_t* frame_buffer,
                const Image& image,
                unsigned factor,
                Rect to,
                std::optional<Rect> from = std::nullopt);

} // namespace painter
