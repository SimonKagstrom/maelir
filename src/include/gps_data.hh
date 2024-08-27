#pragma once

#include <mp-units/systems/international.h>
#include <mp-units/systems/isq.h>
#include <mp-units/systems/si.h>

using namespace mp_units;
using namespace mp_units::si::unit_symbols;
using namespace mp_units::international::unit_symbols;

struct GpsData
{
    double latitude;
    double longitude;

    float speed;
    float heading;

    // Add time, height, etc.
};
