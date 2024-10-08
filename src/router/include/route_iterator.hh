#pragma once

#include "router.hh"

#include <optional>

class RouteIterator
{
public:
    RouteIterator(std::span<const IndexType> route, const IndexType row_size);

    std::optional<Point> Next();

private:
    // See route_iterator_state_machine.pu
    enum class State : uint8_t
    {
        kAtNode,
        kInRoute,
        kEnd,

        kValueCount,
    };


    std::span<const IndexType> m_remaining_route;
    const IndexType m_row_size;

    State m_state {State::kAtNode};
    IndexType m_cur {0};
    Vector m_direction {Vector::Standstill()};
};
