#pragma once

#include "router.hh"
#include "semaphore.hh"

#include <optional>

class IRouteListener
{
public:
    virtual ~IRouteListener() = default;

    virtual void AwakeOn(os::binary_semaphore& semaphore) = 0;

    virtual std::optional<const IndexType> Poll() = 0;
};
