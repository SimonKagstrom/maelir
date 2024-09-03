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

Router::Router(std::span<const uint32_t> land_mask, unsigned row_size)
    : m_land_mask(land_mask)
    , m_row_size(row_size)
{
}

std::span<IndexType>
Router::CalculateRoute(Point from_point, Point to_point)
{
    auto from = PointToLandIndex(from_point, m_row_size);
    auto to = PointToLandIndex(to_point, m_row_size);

    m_open_set.clear();
    m_result.clear();
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
            printf("Found path\n");
            while (cur->parent)
            {
                printf("  Path %d,%d\n", cur->index % m_row_size, cur->index / m_row_size);
                m_result.push_back(cur->index);
                cur = cur->parent;
            }

            return m_result;
        }

        /* Iterate over UP, DOWN, LEFT and RIGHT */
        for (auto neighbor : Neighbors(cur->index))
        {
            auto neighbor_node = GetNode(neighbor.land_index);
            if (!neighbor_node)
            {
                // Max nodes reached?
                continue;
            }

            auto newg = cur->g + 1;

            if ((neighbor_node->IsOpen() || neighbor_node->IsClosed()) && neighbor_node->g <= newg)
            {
                // We have a better value or this is not valid
                continue;
            }

            neighbor_node->parent = cur;
            neighbor_node->g = newg;
            neighbor_node->f = neighbor_node->g + Heuristic(neighbor.land_index, to); // g+h

            printf("Neighbor %d,%d g %d f %d\n",
                   neighbor.land_index % m_row_size,
                   neighbor.land_index / m_row_size,
                   neighbor_node->g,
                   neighbor_node->f);
            if (neighbor_node->IsClosed())
            {
                neighbor_node->closed = false;
            }
            if (!neighbor_node->open)
            {
                printf("PUSH\n");
                m_open_set.push(neighbor_node);
                neighbor_node->open = true;
            }
        }

        cur->Close();
    }

    /* There was no path */
    printf("No path :-(\n");
    return {};
}

Router::Node*
Router::GetNode(IndexType index)
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


etl::vector<FinderNode, 4>
Router::Neighbors(IndexType index) const
{
    etl::vector<FinderNode, 4> neighbors;

    auto above = index - m_row_size;
    if (IsWater(m_land_mask, above))
    {
        neighbors.push_back({above});
    }
    auto below = index + m_row_size;
    if (IsWater(m_land_mask, below))
    {
        neighbors.push_back({below});
    }
    auto left = index - 1;
    if (IsWater(m_land_mask, left))
    {
        neighbors.push_back({left});
    }
    auto right = index + 1;
    if (IsWater(m_land_mask, right))
    {
        neighbors.push_back({right});
    }

    return neighbors;
}

CostType
Router::Heuristic(IndexType from, IndexType to)
{
    int from_x = from % m_row_size;
    int from_y = from / m_row_size;

    int to_x = to % m_row_size;
    int to_y = to / m_row_size;

    // Manhattan distance
    return std::abs(from_x - to_x) + std::abs(from_y - to_y);
}
