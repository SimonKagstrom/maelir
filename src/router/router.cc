#include "router.hh"

namespace
{

IndexType
PointToLandIndex(Point point, unsigned row_size)
{
    return (point.y / kPathFinderTileSize) * row_size + (point.x / kPathFinderTileSize);
}

bool
IsWater(std::span<const uint32_t> land_mask, IndexType index)
{
    if (index / 32 >= land_mask.size())
    {
        printf("Index out of bounds %d (%d) vs %zu\n", index, index / 32, land_mask.size());
        return false;
    }

    return (land_mask[index / 32] & (1 << (index % 32))) == 0;
}

} // namespace

template <size_t CACHE_SIZE>
Router<CACHE_SIZE>::Router(std::span<const uint32_t> land_mask, unsigned height, unsigned width)
    : m_land_mask(land_mask)
    , m_height(height)
    , m_width(width)
{
}

template <size_t CACHE_SIZE>
std::span<IndexType>
Router<CACHE_SIZE>::CalculateRoute(Point from_point, Point to_point)
{
    auto from = PointToLandIndex(from_point, m_width);
    auto to = PointToLandIndex(to_point, m_width);

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
            for (auto rit = m_current_result.rbegin(); rit != m_current_result.rend(); ++rit)
            {
                m_result.push_back(*rit);
            }
            if (rc == Router::AstarResult::kPathFound)
            {
                for (auto cur : m_result)
                {
                    printf("  Path %d,%d\n", cur % m_width, cur / m_width);
                }
                return m_result;
            }
            else
            {
                from = m_current_result.front();
            }
        }
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
            while (cur->parent)
            {
                m_current_result.push_back(cur->index);
                cur = cur->parent;
            }

            return Router::AstarResult::kPathFound;
        }

        /* Iterate over UP, DOWN, LEFT and RIGHT */
        for (auto neighbor_index : Neighbors(cur->index))
        {
            auto neighbor_node = GetNode(neighbor_index);
            if (!neighbor_node)
            {
                while (cur->parent)
                {
                    m_current_result.push_back(cur->index);
                    cur = cur->parent;
                }
                return Router::AstarResult::kMaxNodesReached;
            }

            auto newg = cur->g + 1;

            if ((neighbor_node->IsOpen() || neighbor_node->IsClosed()) && neighbor_node->g <= newg)
            {
                // We have a better value or this is not valid
                continue;
            }

            neighbor_node->parent = cur;
            neighbor_node->g = newg;
            neighbor_node->f = neighbor_node->g + Heuristic(neighbor_index, to); // g+h

            if (neighbor_node->IsClosed())
            {
                neighbor_node->closed = false;
            }
            if (!neighbor_node->open)
            {
                m_open_set.push(neighbor_node);
                neighbor_node->open = true;
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
etl::vector<IndexType, 4>
Router<CACHE_SIZE>::Neighbors(IndexType index) const
{
    etl::vector<IndexType, 4> neighbors;

    auto x = index % m_width;
    auto y = index / m_width;

    if (y > 0)
    {
        auto above = index - m_width;
        if (IsWater(m_land_mask, above))
        {
            neighbors.push_back({above});
        }
    }

    if (y < m_height - 1)
    {
        auto below = index + m_width;
        if (IsWater(m_land_mask, below))
        {
            neighbors.push_back({below});
        }
    }

    if (x > 0)
    {
        auto left = index - 1;
        if (IsWater(m_land_mask, left))
        {
            neighbors.push_back({left});
        }
    }

    if (x < m_width - 1)
    {
        auto right = index + 1;
        if (IsWater(m_land_mask, right))
        {
            neighbors.push_back({right});
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


template class Router<kTargetCacheSize>;
template class Router<kUnitTestCacheSize>;
