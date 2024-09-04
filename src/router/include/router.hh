#pragma once

#include "tile.hh"

#include <etl/priority_queue.h>
#include <etl/unordered_map.h>
#include <etl/vector.h>
#include <queue>
#include <span>

using IndexType = uint32_t;
using CostType = IndexType;

constexpr auto kCacheSize = 1024;

struct FinderNode
{
    IndexType land_index;
};


class Router
{
public:
    Router(std::span<const uint32_t> land_mask, unsigned height, unsigned width);

    std::span<IndexType> CalculateRoute(Point from, Point to);

private:
    enum class AstarResult
    {
        kPathFound,
        kNoPath,
        kMaxNodesReached,
    };

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

    AstarResult RunAstar(IndexType from, IndexType to);

    Node* GetNode(IndexType index);

    etl::vector<FinderNode, 4> Neighbors(IndexType index) const;

    CostType Heuristic(IndexType from, IndexType to);

    const std::span<const uint32_t> m_land_mask;
    const unsigned m_height;
    const unsigned m_width;

    etl::priority_queue<Node*, kCacheSize, etl::vector<Node*, kCacheSize>, CompareNodePointers>
        m_open_set;
    etl::unordered_map<IndexType, Node, kCacheSize> m_nodes;

    std::vector<IndexType> m_current_result;
    std::vector<IndexType> m_result;
};
