#pragma once

#include "tile.hh"

#include <cstdint>

enum class ColorMode : uint8_t
{
    kColor,
    kBlackWhite,
    kBlackRed,

    kValueCount,
};
struct ConfigurationSettings
{
    bool show_speedometer {true};
    ColorMode color_mode {ColorMode::kColor};
    uint8_t minute_average_speed {0};
    uint8_t five_minute_average_speed {0};
    int8_t latitude_adjustment {0}; // In pixels
    int8_t longitude_adjustment {0};

    IndexType home_position {0};
};
