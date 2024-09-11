#include "route_iterator.hh"

#include "route_utils.hh"

RouteIterator::RouteIterator(std::span<const IndexType> route,
                             IndexType top_left,
                             IndexType bottom_right,
                             const IndexType row_size)
    : m_remaining_route(route)
    , m_row_size(row_size)
{
    if (m_remaining_route.empty())
    {
        m_state = State::kEnd;
    }
}

std::optional<IndexType>
RouteIterator::Next()
{
    std::optional<IndexType> out;

    switch (m_state)
    {
    case State::kAtNode:
        out = m_remaining_route.front();

        m_remaining_route = m_remaining_route.subspan(1);

        if (m_remaining_route.empty())
        {
            m_state = State::kEnd;
        }
        else
        {
            m_direction = IndexPairToDirection(out.value(), m_remaining_route.front(), m_row_size);
            m_cur = m_cur + m_direction.dx + m_direction.dy * m_row_size;
            printf("To in route with %d,%d\n", m_cur % m_row_size, m_cur / m_row_size);
            m_state = State::kInRoute;
        }
        break;

    case State::kInRoute: {
        out = m_cur;
        auto next = m_cur + m_direction.dx + m_direction.dy * m_row_size;
        printf("In route with %d,%d to %d,%d\n",
               m_cur % m_row_size,
               m_cur / m_row_size,
               next % m_row_size,
               next / m_row_size);

        if (next == m_remaining_route.front())
        {
            m_state = State::kAtNode;
        }
        else
        {
            m_cur = next;
            // Stay in this state
        }
        break;
    }

    case State::kEnd:
        return std::nullopt;

    case State::kValueCount:
        break;
    }

    return out;
}
