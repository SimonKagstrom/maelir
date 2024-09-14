#include "router.hh"
#include "tile.hh"

static IndexType
PointToLandIndex(Point point, unsigned row_size)
{
    return (point.y / kPathFinderTileSize) * row_size + (point.x / kPathFinderTileSize);
}

static Point
LandIndexToPoint(IndexType index, unsigned row_size)
{
    return {static_cast<int32_t>((index % row_size) * kPathFinderTileSize),
            static_cast<int32_t>((index / row_size) * kPathFinderTileSize)};
}

static bool
IsWater(std::span<const uint32_t> land_mask, IndexType index)
{
    if (index / 32 >= land_mask.size())
    {
        printf("Index out of bounds %d (%d) vs %zu\n", index, index / 32, land_mask.size());
        return false;
    }

    return (land_mask[index / 32] & (1 << (index % 32))) == 0;
}

static Vector
IndexPairToDirection(IndexType from, IndexType to, unsigned row_size)
{
    int from_x = from % row_size;
    int from_y = from / row_size;
    int to_x = to % row_size;
    int to_y = to / row_size;

    return PointPairToDirection({from_x, from_y}, {to_x, to_y});
}
