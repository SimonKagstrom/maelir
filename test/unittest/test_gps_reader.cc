#include "../../src/gps_reader/gps_reader.cc"
#include "gps_utils.hh"
#include "test.hh"

using P = GpsPosition;

namespace
{

consteval MapGpsRasterTile
D(float latitude, float longitude, float latitude_offset = 1, float longitude_offset = 1)
{
    return {.latitude = latitude,
            .longitude = longitude,
            .latitude_offset = latitude_offset,
            .longitude_offset = longitude_offset};
}


class Fixture
{
public:
    Fixture()
    {
        std::ranges::fill(m_backing_store, 0);
        metadata = reinterpret_cast<MapMetadata*>(&m_backing_store[0]);

        constexpr auto kGpsData = std::array {
            // clang-format off
            D(60,16), D(60,17), D(60,18), D(0,0),
            D(0,0),   D(59,17), D(59,18), D(59,19),
            D(0,0),   D(0,0),   D(58,18), D(58,19),
            // clang-format on
        };

        metadata->lowest_longitude = 16;
        metadata->lowest_latitude = 58;
        metadata->highest_longitude = 19;
        metadata->highest_latitude = 60;

        metadata->gps_data_rows = 3;
        metadata->gps_data_row_size = 4;

        metadata->gps_position_offset = sizeof(MapMetadata);

        memcpy(&m_backing_store[metadata->gps_position_offset],
               reinterpret_cast<const uint8_t*>(kGpsData.data()),
               kGpsData.size() * sizeof(MapGpsRasterTile));
    };

    MapMetadata* metadata;

private:
    std::array<uint8_t, 1024> m_backing_store {};
};

} // namespace

TEST_CASE_FIXTURE(Fixture, "the gps-span-from-metadata helper works")
{
    auto s = PositionSpanFromMetadata(*metadata);

    REQUIRE(s.size() == 12);
    REQUIRE(s[0].latitude == 60);
    REQUIRE(s[0].longitude == 16);
}

TEST_CASE_FIXTURE(Fixture, "the map gps tile raster can be translated into points")
{
    auto p = MapGpsTileToPoint(D(60, 16), P {.latitude = 60, .longitude = 16});

    REQUIRE(p.x == 0);
    REQUIRE(p.y == 0);

    p = MapGpsTileToPoint(D(60, 16), P {.latitude = 60.5, .longitude = 16.5});

    REQUIRE(p.x == 0.5 * kGpsPositionSize);
    REQUIRE(p.y == 0.5 * kGpsPositionSize);

    p = MapGpsTileToPoint(D(60, 16, 0.5, 0.5), P {.latitude = 60.5, .longitude = 16.05});

    REQUIRE(p.x == static_cast<uint32_t>(0.1 * kGpsPositionSize));
    REQUIRE(p.y == 1 * kGpsPositionSize);
}

TEST_CASE_FIXTURE(Fixture, "the GPS position converter handles out-of-bounds cases")
{
    auto p_longitude_too_low = gps::PositionToPoint(*metadata, P {.latitude = 59, .longitude = 15});
    auto p_longitude_too_high =
        gps::PositionToPoint(*metadata, P {.latitude = 59, .longitude = 20});
    auto p_latitude_too_low = gps::PositionToPoint(*metadata, P {.latitude = 57, .longitude = 17});
    auto p_latitude_too_high = gps::PositionToPoint(*metadata, P {.latitude = 61, .longitude = 17});

    REQUIRE(p_longitude_too_low == Point {0, 0});
    REQUIRE(p_latitude_too_low == Point {0, 0});
    REQUIRE(p_longitude_too_high == Point {0, 0});
    REQUIRE(p_latitude_too_high == Point {0, 0});
}


TEST_CASE_FIXTURE(Fixture, "the GPS position converter handles in-map cases")
{
    auto t_1_0 = gps::PositionToPoint(*metadata, P {.latitude = 59.1, .longitude = 17.1});

    REQUIRE(t_1_0.x == static_cast<int>(1 * kGpsPositionSize + 0.1 * kGpsPositionSize));
    REQUIRE(t_1_0.y == static_cast<int>(0 * kGpsPositionSize + 0.9 * kGpsPositionSize));

    auto t_2_1 = gps::PositionToPoint(*metadata, P {.latitude = 58.5, .longitude = 18.1});
    REQUIRE(t_2_1.x == static_cast<int>(2 * kGpsPositionSize + 0.1 * kGpsPositionSize));
    REQUIRE(t_2_1.y == static_cast<int>(1 * kGpsPositionSize + 0.5 * kGpsPositionSize));

    auto t_2_1_border = gps::PositionToPoint(*metadata, P {.latitude = 58.9, .longitude = 18.9});
    REQUIRE(t_2_1_border.x == static_cast<int>(2 * kGpsPositionSize + 0.9 * kGpsPositionSize));
    REQUIRE(t_2_1_border.y == static_cast<int>(1 * kGpsPositionSize + 0.1 * kGpsPositionSize));
}
