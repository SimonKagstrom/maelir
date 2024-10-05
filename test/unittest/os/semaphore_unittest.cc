#include "semaphore.hh"

using namespace os;

namespace os
{
struct Impl
{
    Impl(int desired)
        : value(desired)
    {
    }

    int value;
};

} // namespace os


template <ptrdiff_t least_max_value>
counting_semaphore<least_max_value>::counting_semaphore(ptrdiff_t desired) noexcept
{
    m_impl = std::make_unique<Impl>(desired);
}

template <ptrdiff_t least_max_value>
counting_semaphore<least_max_value>::~counting_semaphore()
{
}

template <ptrdiff_t least_max_value>
void
counting_semaphore<least_max_value>::release(ptrdiff_t update) noexcept
{
    m_impl->value += update;
}

template <ptrdiff_t least_max_value>
bool
counting_semaphore<least_max_value>::release_from_isr(ptrdiff_t update) noexcept
{
    release(update);

    return false;
}

template <ptrdiff_t least_max_value>
void
counting_semaphore<least_max_value>::acquire() noexcept
{
    if (m_impl->value == 0)
    {
        return;
    }
    m_impl->value--;
}

template <ptrdiff_t least_max_value>
bool
counting_semaphore<least_max_value>::try_acquire() noexcept
{
    if (m_impl->value == 0)
    {
        return false;
    }
    m_impl->value--;
    return true;
}

template <ptrdiff_t least_max_value>
bool
counting_semaphore<least_max_value>::try_acquire_for_ms(const milliseconds time)
{
    return try_acquire();
}

namespace os
{

template class counting_semaphore<1>;

}
