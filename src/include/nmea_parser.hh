#pragma once

#include "gps_data.hh"

#include <etl/string.h>
#include <etl/vector.h>
#include <optional>
#include <string_view>

class NmeaParser
{
public:
    std::optional<GpsData> PushData(std::string_view data);

private:
    enum class State
    {
        kWaitForDollar,
        kWaitForNewLine,
        kParseLine,

        kValueCount,
    };

    void RunStateMachine(char c);

    void ParseLine(std::string_view line);
    void ParseGppgaData(std::string_view line);

    State m_state {State::kWaitForDollar};
    etl::string<192> m_current_line;

    etl::vector<GpsData, 8> m_pending_data;
};