#pragma once

#include "route_iterator.hh"
#include "route_utils.hh"

#include <ranges>
#include <set>

namespace route_test
{

constexpr auto kRowSize = 16;

// Helpers for comparison
auto
AsSet(auto span)
{
    return std::set(span.begin(), span.end());
}

auto
AsVector(auto span)
{
    return std::vector(span.begin(), span.end());
}


consteval Point
ToPoint(auto row_x, auto row_y)
{
    return Point {row_x * kPathFinderTileSize, row_y * kPathFinderTileSize};
}

consteval IndexType
ToIndex(auto row_x, auto row_y)
{
    return row_y * kRowSize + row_x;
}

constexpr auto
ToXY(auto index)
{
    return Point {static_cast<int32_t>(index % kRowSize), static_cast<int32_t>(index / kRowSize)};
}

auto
IsAdjacent(auto a, auto b)
{
    auto a_xy = ToXY(a);
    auto b_xy = ToXY(b);

    return (a_xy.x == b_xy.x && std::abs(a_xy.y - b_xy.y) == 1) ||
           (a_xy.y == b_xy.y && std::abs(a_xy.x - b_xy.x) == 1);
}

} // namespace route_test
