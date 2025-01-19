#pragma once

#include "image.hh"
#include "rect.hh"
#include "tile.hh"

namespace painter
{

void Blit(uint16_t* frame_buffer, const Image& image, Rect to);

void ZoomedBlit(
    uint16_t* frame_buffer, uint32_t buffer_width, const Image& image, unsigned factor, Rect to);

} // namespace painter
