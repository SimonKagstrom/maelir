#pragma once

#include <trompeloeil/mock.hpp>
#include "../i_route_listener.hh"

class MockRouteListener : public IRouteListener
{
public:
    MAKE_MOCK1(AwakeOn, void(IEventNotifier&));
    MAKE_MOCK0(Poll, std::optional<Event>());
};
