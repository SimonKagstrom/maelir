#pragma once

#include "tile.hh"

#include <etl/priority_queue.h>
#include <etl/unordered_map.h>
#include <etl/vector.h>
#include <queue>
#include <span>

using IndexType = uint32_t;
using CostType = IndexType;

struct FinderNode
{
    IndexType land_index;
};

class Router
{
public:
    Router(std::span<const uint32_t> land_mask, unsigned row_size);

    std::span<IndexType> CalculateRoute(Point from, Point to);

private:
    struct Node
    {
        Node()
            : g(0)
            , f(0)
            , parent(nullptr)
            , open(false)
            , closed(false)
        {
        }

        void Close()
        {
            open = false;
            closed = true;
        }

        void Open()
        {
            open = true;
            closed = false;
        }

        bool IsOpen() const
        {
            return open;
        }

        bool IsClosed() const
        {
            return closed;
        }

        CostType g;
        CostType f;
        IndexType index;
        Node* parent;
        bool open;
        bool closed;
    };

    struct CompareNodePointers
    {
        bool operator()(const Node* lhs, const Node* rhs) const
        {
            return lhs->f > rhs->f;
        }
    };

    Node* GetNode(IndexType index);

    etl::vector<FinderNode, 4> Neighbors(IndexType index) const;

    CostType Heuristic(IndexType from, IndexType to);

    const std::span<const uint32_t> m_land_mask;
    const unsigned m_row_size;
    etl::priority_queue<Node*, 1024, etl::vector<Node*, 1024>, CompareNodePointers> m_open_set;
    etl::unordered_map<IndexType, Node, 1024> m_nodes;

    std::vector<IndexType> m_result;
};
