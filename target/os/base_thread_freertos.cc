#include "base_thread.hh"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/idf_additions.h>

using namespace os;

struct BaseThread::Impl
{
    TaskHandle_t m_task;
};

BaseThread::BaseThread()
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
BaseThread::Start()
{
    xTaskCreate([](void* arg) { static_cast<BaseThread*>(arg)->ThreadLoop(); },
                "x",
                2048,
                this,
                tskIDLE_PRIORITY + 1,
                &m_impl->m_task);
}
