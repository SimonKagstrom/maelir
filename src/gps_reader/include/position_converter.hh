#pragma once

#include "hal/i_gps.hh"
#include "tile.hh"

namespace gps
{

Point PositionToPoint(const MapMetadata& metadata, const GpsPosition& gps_data);
GpsPosition PointToPosition(const MapMetadata& metadata, const Point& pixel_position);

} // namespace gps