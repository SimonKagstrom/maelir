#pragma once

#include <etl/deque.h>

constexpr static auto kMaxStoredPositions = 4;

struct StoredPositions
{
    etl::deque<IndexType, kMaxStoredPositions> positions {};

    bool operator==(const StoredPositions& other) const = default;
    StoredPositions& operator=(const StoredPositions& other) = default;
};
