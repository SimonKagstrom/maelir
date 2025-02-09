#pragma once

#include "tile.hh"

#include <etl/priority_queue.h>
#include <etl/unordered_map.h>
#include <etl/vector.h>
#include <queue>
#include <span>

constexpr auto kTargetCacheSize = 65535;
constexpr auto kUnitTestCacheSize = 24;
constexpr IndexType kInvalidIndex = std::numeric_limits<IndexType>::max();

template <size_t CACHE_SIZE>
class Router
{
    static_assert(CACHE_SIZE <= std::numeric_limits<uint16_t>::max());

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

    std::span<const IndexType> CalculateRoute(Point from, Point to);
    std::span<const IndexType> CalculateRoute(IndexType from, IndexType to);

    // For unit tests
    Stats GetStats() const;

private:
    enum class AstarResult
    {
        kPathFound,
        kNoPath,
        kMaxNodesReached,
    };

    enum class NodeState : uint8_t
    {
        kUnknown,
        kOpen,
        kClosed,
    };

    enum class NeighborType : uint8_t
    {
        kIgnoreLand,
        kAll,
    };

    struct Node
    {
        Node()
            : g(0)
            , f(0)
            , parent(nullptr)
            , state(NodeState::kUnknown)
        {
        }

        void Close()
        {
            state = NodeState::kClosed;
        }

        void Open()
        {
            state = NodeState::kOpen;
        }

        bool IsOpen() const
        {
            return state == NodeState::kOpen;
        }

        bool IsClosed() const
        {
            return state == NodeState::kClosed;
        }

        Node* parent;
        CostType g;
        CostType f;
        IndexType index;
        NodeState state;
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

    etl::vector<IndexType, 8> Neighbors(IndexType index, NeighborType include_neighbors) const;

    CostType Heuristic(IndexType from, IndexType to);

    void ProduceResult(const Node* cur);

    bool AdjacentToLand(const Node* node) const;

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
