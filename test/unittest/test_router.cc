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
         // 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14
            w, w, w, w, w, w, w, w, w, w, w, w, w, w, w, // 0 (make clang-format ignore line breaks)
            w, w, w, w, w, w, w, w, w, w, w, w, w, w, w, // 1
            w, w, w, w, w, w, w, L, L, L, w, w, w, w, w, // 2 Some land
            w, w, w, w, w, w, L, L, w, w, w, w, w, w, w, // 3 Some land
            w, w, w, w, w, w, L, L, L, w, w, w, w, w, w, // 4 Some land
            w, w, w, w, w, w, w, w, L, w, w, w, w, w, w, // 5
            w, w, w, w, w, w, w, w, w, w, w, w, w, w, w, // 6
            w, w, w, w, w, w, w, w, w, w, w, w, w, w, w, // 7
        };

        // Save land mask as uint32_t values
        uint32_t cur_val = 0;
        for (auto i = 0; i < land_mask.size(); ++i)
        {
            cur_val |= land_mask[i] << (i % 32);
            if ((i + 1) % 32 == 0)
            {
                m_land_mask_uint32.push_back(cur_val);
                cur_val = 0;
            }
        }
        if (cur_val != 0)
        {
            m_land_mask_uint32.push_back(cur_val);
        }


        router = std::make_unique<Router>(m_land_mask_uint32, kRowSize);
    }

    auto ToPoint(auto row_x, auto row_y)
    {
        return Point {row_y * kPathFinderTileSize, row_x * kPathFinderTileSize};
    }

    std::unique_ptr<Router> router;
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

    REQUIRE_FALSE(r0.empty());
    REQUIRE_FALSE(r1.empty());
    REQUIRE_FALSE(r2.empty());
    REQUIRE_FALSE(r3.empty());
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
