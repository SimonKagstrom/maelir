#include "router.hh"
#include "test.hh"

constexpr auto kRowSize = 16;

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
            w, w, w, w, w, w, w, w, w, w, w, w, L, w, w, w, // 6
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

    auto ToPoint(auto row_x, auto row_y)
    {
        return Point {row_x * kPathFinderTileSize, row_y * kPathFinderTileSize};
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
    auto r1 = router->CalculateRoute(ToPoint(15, 8), ToPoint(15, 1));

    REQUIRE(r0.empty());
    REQUIRE(r1.empty());
}
