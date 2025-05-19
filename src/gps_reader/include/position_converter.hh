#pragma once

#include "hal/i_gps.hh"
#include "tile.hh"

#include <span>

namespace gps
{

Point PositionToPoint(const MapMetadata& metadata, const GpsPosition& gps_data);
GpsPosition PointToPosition(const MapMetadata& metadata, const Point& pixel_position);
std::span<const MapGpsRasterTile> PositionSpanFromMetadata(const MapMetadata& metadata);

} // namespace gps