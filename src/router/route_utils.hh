#include "router.hh"

static IndexType
PointToLandIndex(Point point, unsigned row_size)
{
    return (point.y / kPathFinderTileSize) * row_size + (point.x / kPathFinderTileSize);
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

static Direction
IndexPairToDirection(IndexType from, IndexType to, unsigned row_size)
{
    auto from_x = from % row_size;
    auto from_y = from / row_size;
    auto to_x = to % row_size;
    auto to_y = to / row_size;

    Direction out = {0, 0};

    if (from_x < to_x)
    {
        out.dx = 1;
    }
    else if (from_x > to_x)
    {
        out.dx = -1;
    }

    if (from_y < to_y)
    {
        out.dy = 1;
    }
    else if (from_y > to_y)
    {
        out.dy = -1;
    }

    return out;
}
