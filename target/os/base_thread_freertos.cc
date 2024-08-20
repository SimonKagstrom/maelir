#include "base_thread.hh"

#include <freertos/FreeRTOS.h>
#include <freertos/idf_additions.h>
#include <freertos/task.h>

using namespace os;

struct BaseThread::Impl
{
    uint8_t m_core;
    TaskHandle_t m_task;
};

BaseThread::BaseThread(uint8_t core)
{
    m_impl = new Impl;
    m_impl->m_core = core;
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
    xTaskCreatePinnedToCore([](void* arg) { static_cast<BaseThread*>(arg)->ThreadLoop(); },
                            "x",
                            2048,
                            this,
                            tskIDLE_PRIORITY + 1,
                            &m_impl->m_task,
                            m_impl->m_core);
}
