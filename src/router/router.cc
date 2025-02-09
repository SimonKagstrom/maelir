#include "router.hh"

#include "route_utils.hh"

#include <fmt/format.h>

template <size_t CACHE_SIZE>
Router<CACHE_SIZE>::Router(std::span<const uint32_t> land_mask, unsigned height, unsigned width)
    : m_land_mask(land_mask)
    , m_height(height)
    , m_width(width)
{
}

template <size_t CACHE_SIZE>
std::span<const IndexType>
Router<CACHE_SIZE>::CalculateRoute(Point from_point, Point to_point)
{
    auto from = PointToLandIndex(from_point, m_width);
    auto to = PointToLandIndex(to_point, m_width);

    return CalculateRoute(from, to);
}

template <size_t CACHE_SIZE>
std::span<const IndexType>
Router<CACHE_SIZE>::CalculateRoute(IndexType from, IndexType to)
{
    if (!IsWater(m_land_mask, from))
    {
        from = FindNearestWater(from);
    }
    if (!IsWater(m_land_mask, to))
    {
        to = FindNearestWater(to);
    }

    m_stats.Reset();
    m_result.clear();

    for (auto i = 0; i < 100; i++)
    {
        auto rc = RunAstar(from, to);
        if (rc == Router::AstarResult::kNoPath)
        {
            return {};
        }
        else
        {
            auto top = kInvalidIndex;

            // When merging, avoid repeating the end node of the previous path
            if (!m_result.empty())
            {
                top = m_result.back();
            }

            for (auto rit = m_current_result.rbegin(); rit != m_current_result.rend(); ++rit)
            {
                if (*rit == top)
                {
                    continue;
                }
                m_result.push_back(*rit);
            }

            if (rc == Router::AstarResult::kPathFound)
            {
                return m_result;
            }
            else
            {
                from = m_current_result.front();
            }
            m_current_result.clear();
        }

        m_stats.partial_paths++;
    }

    return {};
}

template <size_t CACHE_SIZE>
Router<CACHE_SIZE>::AstarResult
Router<CACHE_SIZE>::RunAstar(IndexType from, IndexType to)
{
    m_current_result.clear();
    m_open_set.clear();
    m_nodes.clear();

    auto p = GetNode(from);

    p->f = p->g + Heuristic(from, to); /* g+h */
    p->parent = nullptr;

    m_open_set.push(p);

    /* While there are nodes in the Open set */
    while (!m_open_set.empty())
    {
        auto cur = m_open_set.top();

        m_open_set.pop();

        /* We found a path! */
        if (cur->index == to)
        {
            ProduceResult(cur);

            return Router::AstarResult::kPathFound;
        }

        auto parent_direction = Vector::Standstill();

        if (cur->parent)
        {
            parent_direction = IndexPairToDirection(cur->parent->index, cur->index, m_width);
        }

        /* Iterate over UP, DOWN, LEFT and RIGHT */
        for (auto neighbor_index : Neighbors(cur->index, NeighborType::kIgnoreLand))
        {
            auto neighbor_node = GetNode(neighbor_index);

            m_stats.nodes_expanded++;
            if (!neighbor_node)
            {
                ProduceResult(cur);

                return Router::AstarResult::kMaxNodesReached;
            }

            const auto direction = IndexPairToDirection(cur->index, neighbor_index, m_width);
            const auto is_diagonal = (neighbor_index % m_width != cur->index % m_width) &&
                                     (neighbor_index / m_width != cur->index / m_width);
            auto cost = is_diagonal ? 6 : 4;

            if (direction == parent_direction)
            {
                // Favor straight lines
                cost -= 1;
            }

            // If there's land in this direction, add an extra cost to it to keep the path from land
            if (AdjacentToLand(neighbor_node))
            {
                cost += 8;
            }
            auto newg = cur->g + cost;

            if ((neighbor_node->IsOpen() || neighbor_node->IsClosed()) && neighbor_node->g <= newg)
            {
                // We have a better value or this is not valid
                continue;
            }

            neighbor_node->parent = cur;
            neighbor_node->g = newg;
            neighbor_node->f = neighbor_node->g + Heuristic(neighbor_index, to); // g+h

            if (!neighbor_node->IsOpen())
            {
                m_open_set.push(neighbor_node);
                neighbor_node->Open();
            }
        }

        cur->Close();
    }

    /* There was no path */
    return Router::AstarResult::kNoPath;
}


template <size_t CACHE_SIZE>
Router<CACHE_SIZE>::Node*
Router<CACHE_SIZE>::GetNode(IndexType index)
{
    auto it = m_nodes.find(index);
    if (it != m_nodes.end())
    {
        return &it->second;
    }

    if (m_nodes.full())
    {
        return nullptr;
    }

    auto [insert_it, ok] = m_nodes.insert({index, Node()});
    if (!ok)
    {
        return nullptr;
    }
    insert_it->second.index = index;
    return &insert_it->second;
}


template <size_t CACHE_SIZE>
etl::vector<IndexType, 8>
Router<CACHE_SIZE>::Neighbors(IndexType index, NeighborType include_neighbors) const
{
    etl::vector<IndexType, 8> neighbors;

    auto x = index % m_width;
    auto y = index / m_width;

    for (auto dy = -1; dy <= 1; dy++)
    {
        for (auto dx = -1; dx <= 1; dx++)
        {
            if (dx == 0 && dy == 0)
            {
                // Skip self
                continue;
            }

            auto nx = x + dx;
            auto ny = y + dy;

            if (nx < 0 || nx >= m_width || ny < 0 || ny >= m_height)
            {
                continue;
            }

            auto neighbor = ny * m_width + nx;
            if (include_neighbors == NeighborType::kIgnoreLand)
            {
                if (IsWater(m_land_mask, neighbor))
                {
                    neighbors.push_back(neighbor);
                }
            }
            else
            {
                neighbors.push_back(neighbor);
            }
        }
    }

    return neighbors;
}


template <size_t CACHE_SIZE>
CostType
Router<CACHE_SIZE>::Heuristic(IndexType from, IndexType to)
{
    int from_x = from % m_width;
    int from_y = from / m_width;

    int to_x = to % m_width;
    int to_y = to / m_width;

    // Euclidean distance
    //return std::sqrt(std::pow(from_x - to_x, 2) + std::pow(from_y - to_y, 2));

    // Manhattan distance
    return std::abs(from_x - to_x) + std::abs(from_y - to_y);
}

template <size_t CACHE_SIZE>
bool
Router<CACHE_SIZE>::AdjacentToLand(const Node* node) const
{
    for (auto neighbor_index : Neighbors(node->index, NeighborType::kAll))
    {
        if (!IsWater(m_land_mask, neighbor_index))
        {
            return true;
        }
    }

    return false;
}

template <size_t CACHE_SIZE>
IndexType
Router<CACHE_SIZE>::FindNearestWater(IndexType from) const
{
    constexpr auto kLimit = 16;
    auto point = LandIndexToPoint(from, m_width);

    // Find land
    for (auto dx = 0; dx < kLimit; dx++)
    {
        for (auto dy = 0; dy < kLimit; dy++)
        {
            auto nx = point.x + dx * kPathFinderTileSize;
            auto ny = point.y + dy * kPathFinderTileSize;

            auto bx = point.x - dx * kPathFinderTileSize;
            auto by = point.y - dy * kPathFinderTileSize;

            auto neighbor = PointToLandIndex({nx, ny}, m_width);
            auto below_neighbor = PointToLandIndex({bx, by}, m_width);
            if (IsWater(m_land_mask, neighbor))
            {
                return neighbor;
            }
            if (IsWater(m_land_mask, below_neighbor))
            {
                return below_neighbor;
            }
        }
    }

    // Give up
    return from;
}

template <size_t CACHE_SIZE>
void
Router<CACHE_SIZE>::ProduceResult(const Node* cur)
{
    auto last_direction = Vector::Standstill();

    while (cur->parent)
    {
        auto next_direction = IndexPairToDirection(cur->index, cur->parent->index, m_width);

        if (next_direction != last_direction)
        {
            // Can happen if multiple paths are merged
            if (m_current_result.size() > 0 && m_current_result.back() == cur->index)
            {
                m_current_result.pop_back();
            }

            m_current_result.push_back(cur->index);
        }
        cur = cur->parent;
        if (!cur->parent)
        {
            // Always push the last
            m_current_result.push_back(cur->index);
        }
        last_direction = next_direction;
    }
}

template <size_t CACHE_SIZE>
Router<CACHE_SIZE>::Stats
Router<CACHE_SIZE>::GetStats() const
{
    return m_stats;
}

template class Router<kTargetCacheSize>;
template class Router<kUnitTestCacheSize>;
