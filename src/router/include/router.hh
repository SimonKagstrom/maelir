#pragma once

#include "tile.hh"

#include <etl/priority_queue.h>
#include <etl/unordered_map.h>
#include <etl/vector.h>
#include <queue>
#include <span>

using IndexType = uint32_t;
using CostType = IndexType;

constexpr auto kTargetCacheSize = 4096;
constexpr auto kUnitTestCacheSize = 20;

template <size_t CACHE_SIZE>
class Router
{
public:
    struct Stats
    {
        void Reset()
        {
            partial_paths = 0;
            nodes_expanded = 0;
        }

        unsigned partial_paths {0};
        unsigned nodes_expanded {0};
    };

    Router(std::span<const uint32_t> land_mask, unsigned height, unsigned width);

    std::span<IndexType> CalculateRoute(Point from, Point to);

    // For unit tests
    Stats GetStats() const;

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

    etl::vector<IndexType, 8> Neighbors(IndexType index) const;

    CostType Heuristic(IndexType from, IndexType to);

    const std::span<const uint32_t> m_land_mask;
    const unsigned m_height;
    const unsigned m_width;

    etl::priority_queue<Node*, CACHE_SIZE, etl::vector<Node*, CACHE_SIZE>, CompareNodePointers>
        m_open_set;
    etl::unordered_map<IndexType, Node, CACHE_SIZE> m_nodes;

    std::vector<IndexType> m_current_result;
    std::vector<IndexType> m_result;
    Stats m_stats;
};