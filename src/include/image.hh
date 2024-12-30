#pragma once

#include <cstdint>
#include <lvgl.h>
#include <span>

class Image
{
public:
    Image(std::span<const uint16_t> data, uint16_t width, uint16_t height)
        : data(data)
        , width(width)
        , height(height)
    {
        lv_image_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
        lv_image_dsc.data_size = width * height * sizeof(uint16_t);
        lv_image_dsc.header.w = width;
        lv_image_dsc.header.h = height;
        lv_image_dsc.header.flags = 0;
        lv_image_dsc.header.stride = width * sizeof(uint16_t);
        lv_image_dsc.data = reinterpret_cast<const uint8_t*>(data.data());
        lv_image_dsc.header.cf = LV_COLOR_FORMAT_NATIVE;
    }

    std::span<const uint16_t> data;
    uint16_t width;
    uint16_t height;

    lv_image_dsc_t lv_image_dsc;
};
