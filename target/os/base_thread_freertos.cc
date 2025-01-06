#include "base_thread.hh"
#include "time.hh"

#include <freertos/FreeRTOS.h>
#include <freertos/idf_additions.h>
#include <freertos/task.h>

using namespace os;

struct BaseThread::Impl
{
    TaskHandle_t m_task;
};

BaseThread::BaseThread()
    : m_timer_manager(m_semaphore)
{
    m_impl = new Impl;
    m_impl->m_task = nullptr;
}

BaseThread::~BaseThread()
{
    assert(false);

    delete m_impl;
}

void
BaseThread::Start(uint8_t core, ThreadPriority priority, uint32_t stack_size)
{
    static_assert(tskIDLE_PRIORITY == 0, "FreeRTOS priority assumption broken");
    assert(priority < configMAX_PRIORITIES);
    assert(priority > 0);

    xTaskCreatePinnedToCore([](void* arg) { static_cast<BaseThread*>(arg)->ThreadLoop(); },
                            "x",
                            stack_size,
                            this,
                            priority,
                            &m_impl->m_task,
                            core);
}


uint32_t
os::GetTimeStampRaw()
{
    return pdTICKS_TO_MS(xTaskGetTickCount());
}

milliseconds
os::GetTimeStamp()
{
    auto ms_count = os::GetTimeStampRaw();

    return milliseconds(ms_count);
}

void
os::Sleep(milliseconds delay)
{
    vTaskDelay(pdMS_TO_TICKS(delay.count()));
}
