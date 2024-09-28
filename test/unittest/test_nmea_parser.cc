#include "nmea_parser.hh"
#include "test.hh"

TEST_CASE("the NMEA parser can pass GPS data in the sunny-day case")
{
    NmeaParser parser;

    auto data =
        parser.PushData("$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\n");

    REQUIRE(data);
    REQUIRE(data->position->latitude == doctest::Approx(48.1173));
    REQUIRE(data->position->longitude == doctest::Approx(11.5167));
}

TEST_CASE("the NMEA parser works if data is split up")
{
    NmeaParser parser;

    auto data = parser.PushData("$GPGGA,123519,4807.038,N");
    REQUIRE(data == std::nullopt);

    data = parser.PushData(",01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\n");

    REQUIRE(data);
    REQUIRE(data->position->latitude == doctest::Approx(48.1173));
    REQUIRE(data->position->longitude == doctest::Approx(11.5167));
}

TEST_CASE("the NMEA parser can handle broken messages")
{
    NmeaParser parser;

    THEN("an oversized message is thrown")
    {
        std::string buf;

        std::copy("$GPGGA", "$GPGGA", std::back_inserter(buf));
        for (auto i = 0; i < 1024; i++)
        {
            buf.push_back('x');
        }
        buf.push_back('\n');

        auto data = parser.PushData(buf);
        REQUIRE_FALSE(data);
    }
}

TEST_CASE("the NMEA parser can restart after broken messages")
{
    NmeaParser parser;

    // An extra dollar
    auto data =
        parser.PushData("$GPGGA,123519,4807.038,N,01131.000,$E,1,08,0.9,545.4,M,46.9,M,,*47$GPGGA,"
                        "123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\n");
    REQUIRE(data);
    REQUIRE(data->position->latitude == doctest::Approx(48.1173));
    REQUIRE(data->position->longitude == doctest::Approx(11.5167));
}


TEST_CASE("an empty NMEA message is handled")
{
    //"$GPGGA,171029.00,,,    "
}