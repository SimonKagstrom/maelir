#pragma once

#include "time.hh"

#include <memory>
#include <semaphore>

namespace os
{
struct Impl;

// From std::semaphore
template <ptrdiff_t least_max_value = INT32_MAX>
class counting_semaphore
{
    std::unique_ptr<Impl> m_impl;

public:
    explicit counting_semaphore(ptrdiff_t desired) noexcept;

    ~counting_semaphore();

    counting_semaphore(const counting_semaphore&) = delete;
    counting_semaphore& operator=(const counting_semaphore&) = delete;

    void release(ptrdiff_t __update = 1) noexcept;

    // Return true if a higher prio task was awoken
    bool release_from_isr(ptrdiff_t __update = 1) noexcept;

    void acquire() noexcept;

    bool try_acquire() noexcept;

    template <typename _Rep, typename _Period>
    bool try_acquire_for(const std::chrono::duration<_Rep, _Period>& rtime)
    {
        return try_acquire_for_ms(rtime);
    }

    bool try_acquire_for_ms(const milliseconds rtime);
};

using binary_semaphore = counting_semaphore<1>;

} // namespace os
