#pragma once

#include "gps_data.hh"
#include "tile.hh"

namespace gps
{

Point PositionToPoint(const MapMetadata& metadata, const GpsPosition& gps_data);
GpsPosition PointToPosition(const MapMetadata& metadata, const Point& pixel_position);

} // namespace gps