#pragma once

#include <trompeloeil/mock.hpp>
#include "../gps_port.hh"

class MockGpsPort : public IGpsPort
{
public:
    MAKE_MOCK0(Poll, std::optional<GpsData>());
    MAKE_MOCK1(DoAwakeOn, void(os::binary_semaphore*));
};
