// https://aprs.gids.nl/nmea
#include "nmea_parser.hh"

#include <ranges>

namespace
{

std::optional<float>
ToDegrees(auto word)
{
    if (word.empty())
    {
        return std::nullopt;
    }

    char* end = nullptr;
    auto nmea = std::strtod(word.data(), &end);
    if (end == nullptr)
    {
        return std::nullopt;
    }

    int deg = static_cast<int>(nmea) / 100;
    float min = nmea - (deg * 100);
    nmea = deg + min / 60.0f;

    return nmea;
}

} // namespace

std::optional<hal::RawGpsData>
NmeaParser::PushData(std::string_view data)
{
    for (auto c : data)
    {
        RunStateMachine(c);
    }

    if (!m_pending_data.empty())
    {
        auto data = m_pending_data.back();
        m_pending_data.clear();

        return data;
    }
    return std::nullopt;
}


void
NmeaParser::RunStateMachine(char c)
{
    auto before = m_state;

    do
    {
        before = m_state;
        switch (m_state)
        {
        case State::kWaitForDollar:
            m_current_line.clear();

            if (c == '$')
            {
                m_state = State::kWaitForNewLine;
                break;
            }
            break;
        case State::kWaitForNewLine:
            if (m_current_line.full())
            {
                m_state = State::kWaitForDollar;
                break;
            }

            m_current_line.push_back(c);

            if (c == '\n')
            {
                m_state = State::kParseLine;
                break;
            }
            break;
        case State::kParseLine:
            ParseLine(std::string_view(m_current_line.data(), m_current_line.size()));
            m_state = State::kWaitForDollar;
            break;

        case State::kValueCount:
            break;
        }
    } while (before != m_state);
}

void
NmeaParser::ParseLine(std::string_view line)
{
    if (line.starts_with("$GPGGA,"))
    {
        // $GPGGA,174558.00,5917.60788,N,01757.40192,E,1,08,1.08,48.3,M,24.6,M,,*69
        ParseGppgaData(line.substr(7));
    }
    else if (line.starts_with("$GPVTG,"))
    {
        // $GPVTG,360.0,T,348.7,M,000.0,N,000.0,K*43
        ParseGpvtgData(line.substr(7));
    }
}

void
NmeaParser::ParseGppgaData(std::string_view line)
{
    auto index = 0;
    std::optional<float> latitude;
    std::optional<float> longitude;

    // time      latitude  N/S longitude E/W fix sat hdop altitude M height M time checksum
    // 174558.00,5917.60788,N,01757.40192,E,1,08,1.08,48.3,M,24.6,M,,*69
    constexpr auto kTimeIndex = 0;
    constexpr auto kLatitudeIndex = 1;
    constexpr auto kLatitudeDirectionIndex = 2;
    constexpr auto kLongitudeIndex = 3;
    constexpr auto kLongitudeDirectionIndex = 4;
    constexpr auto kFixIndex = 5;
    constexpr auto kSatIndex = 6;
    constexpr auto kHdopIndex = 7;
    constexpr auto kAltitudeIndex = 8;
    constexpr auto kHeightIndex = 10;
    constexpr auto kTimeSinceLastDgpsUpdateIndex = 12;
    constexpr auto kChecksumIndex = 13;

    auto valid = false;
    for (const auto sub_range : std::views::split(line, ','))
    {
        const auto word = std::string_view(sub_range.begin(), sub_range.end());

        switch (index)
        {
        case kLatitudeIndex:
            latitude = ToDegrees(word);
            break;
        case kLatitudeDirectionIndex:
            if (word == "S")
            {
                latitude = -latitude.value();
            }
            break;
        case kLongitudeIndex:
            longitude = ToDegrees(word);
            break;
        case kLongitudeDirectionIndex:
            if (word == "W")
            {
                longitude = -longitude.value();
            }
            break;
        case kFixIndex:
            valid = word == "1" || word == "2";
            break;
        default:
            break;
        }

        index++;
    }

    if (valid && latitude && longitude)
    {
        hal::RawGpsData cur;

        cur.position = {latitude.value(), longitude.value()};
        m_pending_data.push_back(cur);
    }
}

void
NmeaParser::ParseGpvtgData(std::string_view line)
{
    auto index = 0;
    std::optional<float> course;
    std::optional<float> speed;

    // course T, course M, speed N, speed K
    // 360.0,T,348.7,M,000.0,N,000.0,K*43
    for (const auto sub_range : std::views::split(line, ','))
    {
        const auto word = std::string_view(sub_range.begin(), sub_range.end());
        char* end;

        if (index == 0)
        {
            course = std::strtof(word.data(), &end);
        }
        else if (index == 4)
        {
            speed = std::strtof(word.data(), &end);
        }

        index++;
    }

    if (speed && course)
    {
        hal::RawGpsData cur;

        cur.heading = course.value();
        cur.speed = speed.value();

        m_pending_data.push_back(cur);
    }
}
