#pragma once

#include <trompeloeil/mock.hpp>
#include "../i_route_listener.hh"

class MockRouteListener : public IRouteListener
{
public:
    MAKE_MOCK1(AwakeOn, void(os::binary_semaphore&));
    MAKE_MOCK0(Poll, std::optional<Event>());
};
