#include "semaphore.hh"

#include <QSemaphore>

using namespace os;

namespace os
{
struct Impl
{
    Impl(int desired)
        : m_sem(desired)
    {
    }

    QSemaphore m_sem;
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
    m_impl->m_sem.release(update);
}

template <ptrdiff_t least_max_value>
void
counting_semaphore<least_max_value>::acquire() noexcept
{
    m_impl->m_sem.acquire();
}

template <ptrdiff_t least_max_value>
bool
counting_semaphore<least_max_value>::try_acquire() noexcept
{
    return m_impl->m_sem.tryAcquire();
}

template <ptrdiff_t least_max_value>
bool
counting_semaphore<least_max_value>::try_acquire_for_ms(const milliseconds time)
{
    return m_impl->m_sem.tryAcquire(1, time.count());
}

namespace os
{

template class counting_semaphore<1>;

}
