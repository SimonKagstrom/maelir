#pragma once

#include <cstdint>
#include <lvgl.h>
#include <span>

class Image
{
public:
    Image(std::span<const uint8_t> data, uint16_t width, uint16_t height, bool has_alpha = false)
        : data(data)
        , m_data16_size(data.size() / sizeof(uint16_t))
    {
        auto pixel_size = sizeof(uint16_t) + (has_alpha ? 2 : 0);

        lv_image_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
        lv_image_dsc.header.w = width;
        lv_image_dsc.header.h = height;
        lv_image_dsc.header.flags = 0;
        lv_image_dsc.header.stride = width * pixel_size;
        lv_image_dsc.header.cf = has_alpha ? LV_COLOR_FORMAT_ARGB8888 : LV_COLOR_FORMAT_NATIVE;

        lv_image_dsc.data_size = width * height * pixel_size;
        lv_image_dsc.data = reinterpret_cast<const uint8_t*>(data.data());
    }

    auto Width() const
    {
        return lv_image_dsc.header.w;
    }

    auto Height() const
    {
        return lv_image_dsc.header.h;
    }

    const std::span<const uint16_t> Data16() const
    {
        return {reinterpret_cast<const uint16_t*>(data.data()), m_data16_size};
    }

    std::span<const uint8_t> data;

    lv_image_dsc_t lv_image_dsc;

private:
    const size_t m_data16_size;
};
