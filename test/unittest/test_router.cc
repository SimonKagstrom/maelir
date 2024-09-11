#include "route_iterator.hh"
#include "route_utils.hh"
#include "router.hh"
#include "test.hh"

#include <fmt/format.h>
#include <set>

constexpr auto kRowSize = 16;

namespace
{

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


consteval auto
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
    return Point {index % kRowSize, index / kRowSize};
}


} // namespace

class Fixture
{
public:
    Fixture()
    {
        constexpr auto L = true;
        constexpr auto w = false;

        // 16x8 land mask
        land_mask = {
            // clang-format off
        //  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
            w, w, w, w, w, w, w, w, w, w, w, w, w, w, w, w, // 0
            w, w, w, w, w, w, w, w, w, w, w, w, w, w, w, w, // 1
            w, w, w, w, w, w, w, L, L, L, w, w, w, w, w, w, // 2
            w, w, w, w, w, w, L, L, w, w, w, w, w, w, w, w, // 3 Some land
            w, w, w, w, w, w, L, L, L, w, w, w, w, w, w, w, // 4
            w, w, w, w, w, w, w, w, L, w, w, w, L, L, L, L, // 5 Walled-in land
            L, L, L, L, L, L, L, w, w, w, w, w, L, w, w, w, // 6
            w, w, w, w, w, w, w, w, w, w, w, w, L, w, w, w, // 7
            w, w, w, w, w, w, w, w, w, w, w, w, L, w, w, w, // 8
            // clang-format on
        };

        // Save land mask as uint32_t values
        uint32_t cur_val = 0;
        int i;
        for (i = 0; i < land_mask.size(); ++i)
        {
            cur_val |= land_mask[i] << (i % 32);
            if ((i + 1) % 32 == 0)
            {
                m_land_mask_uint32.push_back(cur_val);
                cur_val = 0;
            }
        }
        if (land_mask.size() % 32 != 0)
        {
            // Fill the remaining bits with 1s (as land)
            cur_val |= 0xffffffff >> (i % 32);
            m_land_mask_uint32.push_back(cur_val);
        }


        router = std::make_unique<Router<kUnitTestCacheSize>>(m_land_mask_uint32, 8, kRowSize);
    }

    std::unique_ptr<Router<kUnitTestCacheSize>> router;
    std::vector<bool> land_mask;

private:
    std::vector<uint32_t> m_land_mask_uint32;
};


TEST_CASE_FIXTURE(Fixture, "the router can find adjacent paths")
{
    auto r0 = router->CalculateRoute(ToPoint(0, 0), ToPoint(0, 1));
    auto r1 = router->CalculateRoute(ToPoint(0, 1), ToPoint(0, 0));
    auto r2 = router->CalculateRoute(ToPoint(1, 0), ToPoint(0, 0));
    auto r3 = router->CalculateRoute(ToPoint(0, 0), ToPoint(1, 0));
    auto r4 = router->CalculateRoute(ToPoint(14, 7), ToPoint(15, 7));

    REQUIRE_FALSE(r0.empty());
    REQUIRE_FALSE(r1.empty());
    REQUIRE_FALSE(r2.empty());
    REQUIRE_FALSE(r3.empty());
    REQUIRE_FALSE(r4.empty());
}

TEST_CASE_FIXTURE(Fixture, "the router can find non-adjacent (but without blocks) paths")
{
    auto r0 = router->CalculateRoute(ToPoint(0, 0), ToPoint(0, 2));
    auto r1 = router->CalculateRoute(ToPoint(0, 0), ToPoint(1, 2));

    REQUIRE_FALSE(r0.empty());
    REQUIRE_FALSE(r1.empty());
}

TEST_CASE_FIXTURE(Fixture, "the router can route around obstacles")
{
    auto r0 = router->CalculateRoute(ToPoint(8, 3), ToPoint(4, 3));

    REQUIRE_FALSE(r0.empty());
}


TEST_CASE_FIXTURE(Fixture, "when walled in, the router can't find a path")
{
    auto r0 = router->CalculateRoute(ToPoint(15, 8), ToPoint(4, 8));
    REQUIRE(router->GetStats().partial_paths == 0);
    auto r1 = router->CalculateRoute(ToPoint(15, 8), ToPoint(15, 1));

    REQUIRE(r0.empty());
    REQUIRE(r1.empty());
}


TEST_CASE_FIXTURE(Fixture, "the router can merge partial paths")
{
    auto r0 = router->CalculateRoute(ToPoint(0, 7), ToPoint(0, 5));

    REQUIRE_FALSE(r0.empty());
    REQUIRE(router->GetStats().partial_paths > 0);
}


TEST_CASE_FIXTURE(Fixture, "the router can do diagonal paths")
{
    auto r0 = router->CalculateRoute(ToPoint(0, 0), ToPoint(3, 3));

    REQUIRE_FALSE(r0.empty());
    REQUIRE(AsSet(r0) == AsSet(std::array {ToIndex(0, 0), ToIndex(3, 3)}));
}


TEST_CASE_FIXTURE(Fixture, "the router reports only direction changes in it's paths")
{
    auto r0 = router->CalculateRoute(ToPoint(0, 0), ToPoint(5, 0));

    REQUIRE(r0.size() == 2);
    REQUIRE(AsVector(r0) == AsVector(std::array {ToIndex(0, 0), ToIndex(5, 0)}));

    // r0 is a span, so now invalid
    r0 = router->CalculateRoute(ToPoint(7, 1), ToPoint(8, 3));
    REQUIRE(r0.size() == 5);

    REQUIRE(AsVector(r0) ==
            AsVector(std::array {
                ToIndex(7, 1), ToIndex(9, 1), ToIndex(10, 2), ToIndex(9, 3), ToIndex(8, 3)}));

    r0 = router->CalculateRoute(ToPoint(15, 3), ToPoint(15, 0));

    REQUIRE(r0.size() == 2);
    REQUIRE(AsVector(r0) == AsVector(std::array {ToIndex(15, 3), ToIndex(15, 0)}));
}



TEST_CASE_FIXTURE(Fixture, "Indices can be translated to directions")
{
    auto d_standstill = IndexPairToDirection(ToIndex(1, 0), ToIndex(1, 0), kRowSize);
    auto d_down = IndexPairToDirection(ToIndex(0, 0), ToIndex(0, 1), kRowSize);
    auto d_down_far = IndexPairToDirection(ToIndex(0, 0), ToIndex(0, 2), kRowSize);
    auto d_diagonal = IndexPairToDirection(ToIndex(0, 0), ToIndex(1, 1), kRowSize);
    auto d_diagonal_left = IndexPairToDirection(ToIndex(3, 3), ToIndex(0, 0), kRowSize);
    auto d_right = IndexPairToDirection(ToIndex(0, 0), ToIndex(3, 0), kRowSize);
    auto d_up = IndexPairToDirection(ToIndex(3, 3), ToIndex(3, 1), kRowSize);

    REQUIRE(d_standstill == Direction::StandStill());
    REQUIRE(d_down == Direction {0, 1});
    REQUIRE(d_down_far == Direction {0, 1});
    REQUIRE(d_diagonal == Direction {1, 1});
    REQUIRE(d_diagonal_left == Direction {-1, -1});
    REQUIRE(d_right == Direction {1, 0});
    REQUIRE(d_up == Direction {0, -1});
}




TEST_CASE_FIXTURE(Fixture, "the route iterator handles corner cases")
{
    auto empty_it = RouteIterator({}, ToIndex(0, 0), ToIndex(15, 8), kRowSize);
    REQUIRE(empty_it.Next() == std::nullopt);

    auto single_node = std::array {ToIndex(0, 0)};
    auto single_it = RouteIterator(single_node, ToIndex(0, 0), ToIndex(15, 8), kRowSize);
    REQUIRE(single_it.Next() == ToIndex(0, 0));
    REQUIRE(single_it.Next() == std::nullopt);
}

TEST_CASE_FIXTURE(Fixture, "the route iterator handles regular iteration")
{
    auto single_route = std::array {ToIndex(0, 0), ToIndex(0, 2)};
    auto single_it = RouteIterator(single_route, ToIndex(0, 0), ToIndex(15, 8), kRowSize);

    REQUIRE(single_it.Next() == ToIndex(0, 0));
    REQUIRE(single_it.Next() == ToIndex(0, 1));
    REQUIRE(single_it.Next() == ToIndex(0, 2));
    REQUIRE(single_it.Next() == std::nullopt);
}

TEST_CASE_FIXTURE(Fixture, "the route iterator handles turns")
{
    // Right-down, down, right
    auto single_route = std::array {ToIndex(0, 0), ToIndex(1, 1), ToIndex(1, 3), ToIndex(2, 3)};
    auto single_it = RouteIterator(single_route, ToIndex(0, 0), ToIndex(15, 8), kRowSize);

    REQUIRE(*single_it.Next() == ToIndex(0, 0));
    REQUIRE(*single_it.Next() == ToIndex(1, 1));
    REQUIRE(*single_it.Next() == ToIndex(1, 2));
    REQUIRE(*single_it.Next() == ToIndex(1, 3));
    REQUIRE(*single_it.Next() == ToIndex(2, 3));
    REQUIRE(single_it.Next() == std::nullopt);

    // up, left, down
    auto next_route = std::array {ToIndex(8, 8), ToIndex(8, 5), ToIndex(6, 5), ToIndex(6, 4)};
    auto next_it = RouteIterator(next_route, ToIndex(0, 0), ToIndex(15, 8), kRowSize);

    REQUIRE(*next_it.Next() == ToIndex(8, 8));
    REQUIRE(*next_it.Next() == ToIndex(8, 7));
    REQUIRE(*next_it.Next() == ToIndex(8, 6));
    REQUIRE(*next_it.Next() == ToIndex(8, 5));
    REQUIRE(*next_it.Next() == ToIndex(7, 5));
    REQUIRE(*next_it.Next() == ToIndex(6, 5));
    REQUIRE(*next_it.Next() == ToIndex(6, 4));
    REQUIRE(next_it.Next() == std::nullopt);
}
