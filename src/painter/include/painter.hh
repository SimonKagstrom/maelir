#pragma once

#include "image.hh"
#include "tile.hh"

namespace painter
{

struct Rect
{
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
};


void Blit(uint16_t* frame_buffer, const Image& image, Rect to);

void ZoomedBlit(
    uint16_t* frame_buffer, uint32_t buffer_width, const Image& image, unsigned factor, Rect to);

} // namespace painter
