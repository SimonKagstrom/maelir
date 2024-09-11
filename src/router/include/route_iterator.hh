#pragma once

#include "router.hh"

#include <optional>

class RouteIterator
{
public:
    RouteIterator(std::span<const IndexType> route,
                  IndexType top_left,
                  IndexType bottom_right,
                  const IndexType row_size);

    std::optional<IndexType> Next();

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
    Direction m_direction {Direction::StandStill()};
};
