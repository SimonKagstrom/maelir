#pragma once

#include "semaphore.hh"
#include "time.hh"

#include <optional>

class BaseThread
{
public:
    virtual ~BaseThread() = default;

    void Awake();

    void Start();

protected:
    /// @brief the thread has been awoken
    virtual std::optional<milliseconds> OnActivation() = 0;
};
