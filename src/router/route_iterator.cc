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
    while (true)
    {
        switch (m_state)
        {
        case State::kAtNode:
            m_cur = m_remaining_route.front();

            m_remaining_route = m_remaining_route.subspan(1);

            if (m_remaining_route.empty())
            {
                m_state = State::kEnd;
            }
            else
            {
                m_direction = IndexPairToDirection(m_cur, m_remaining_route.front(), m_row_size);
                m_state = State::kInRoute;
            }

            return m_cur;

        case State::kInRoute: {
            m_cur = m_cur + m_direction.dx + m_direction.dy * m_row_size;

            if (m_cur == m_remaining_route.front())
            {
                m_state = State::kAtNode;
                break;
            }

            return m_cur;
        }

        case State::kEnd:
            return std::nullopt;

        case State::kValueCount:
            // Unrechable
            return std::nullopt;
        }
    }

    // Unrechable
    return std::nullopt;
}
