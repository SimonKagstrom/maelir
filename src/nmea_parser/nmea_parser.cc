#include "nmea_parser.hh"

#include <fmt/format.h>
#include <ranges>

namespace
{

auto
ToDegrees(auto nmea)
{
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
    if (!line.starts_with("$GPGGA,"))
    {
        return;
    }
    // $GPGGA,174558.00,5917.60788,N,01757.40192,E,1,08,1.08,48.3,M,24.6,M,,*69
    ParseGppgaData(line.substr(7));
}

void
NmeaParser::ParseGppgaData(std::string_view line)
{
    auto index = 0;
    std::optional<double> latitude;
    std::optional<double> longitude;

    // time      latitude  N/S longitude E/W fix sat hdop altitude M height M time diff
    // 174558.00,5917.60788,N,01757.40192,E,1,08,1.08,48.3,M,24.6,M,,*69
    for (const auto sub_range : std::views::split(line, ','))
    {
        const auto word = std::string_view(sub_range.begin(), sub_range.end());

        if (index == 1)
        {
            char* end;
            latitude = ToDegrees(std::strtod(word.data(), &end));
        }
        else if (index == 2)
        {
            if (word == "S")
            {
                latitude = -latitude.value();
            }
        }

        if (index == 3)
        {
            char* end;
            longitude = ToDegrees(std::strtod(word.data(), &end));
        }
        else if (index == 4)
        {
            if (word == "W")
            {
                longitude = -longitude.value();
            }
        }
        index++;
    }

    if (latitude && longitude)
    {
        m_pending_data.push_back(hal::RawGpsData {{latitude.value(), longitude.value()}});
    }
}
