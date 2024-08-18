#include "semaphore.hh"

#include <cassert>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

using namespace os;

namespace os
{
struct Impl
{
    Impl(auto least_max_value, auto desired)
    {
        m_sem = xSemaphoreCreateCounting(least_max_value, desired);
    }

    ~Impl()
    {
        vSemaphoreDelete(m_sem);
    }

    SemaphoreHandle_t m_sem;
};

} // namespace os


template <ptrdiff_t least_max_value>
counting_semaphore<least_max_value>::counting_semaphore(ptrdiff_t desired) noexcept
{
    m_impl = std::make_unique<Impl>(least_max_value, desired);
}

template <ptrdiff_t least_max_value>
counting_semaphore<least_max_value>::~counting_semaphore()
{
}

template <ptrdiff_t least_max_value>
void
counting_semaphore<least_max_value>::release(ptrdiff_t update) noexcept
{
    if (xPortInIsrContext())
    {
        BaseType_t woken;
        xSemaphoreGiveFromISR(m_impl->m_sem, &woken);
    }
    else
    {
        xSemaphoreGive(m_impl->m_sem);
    }
}

template <ptrdiff_t least_max_value>
void
counting_semaphore<least_max_value>::acquire() noexcept
{
    xSemaphoreTake(m_impl->m_sem, portMAX_DELAY);
}

template <ptrdiff_t least_max_value>
bool
counting_semaphore<least_max_value>::try_acquire() noexcept
{
    return xSemaphoreTake(m_impl->m_sem, 0) == pdTRUE;
}

template <ptrdiff_t least_max_value>
bool
counting_semaphore<least_max_value>::try_acquire_for_ms(const milliseconds time)
{
    return xSemaphoreTake(m_impl->m_sem, portTICK_PERIOD_MS * time.count()) == pdTRUE;
}

namespace os
{

template class counting_semaphore<1>;

}
