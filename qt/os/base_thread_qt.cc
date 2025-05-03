#include "base_thread.hh"

#include <QThread>

using namespace os;

struct BaseThread::Impl
{
    QThread* m_thread;
};

BaseThread::BaseThread()
    : m_timer_manager(m_semaphore)
{
    m_impl = new Impl;
    m_impl->m_thread = QThread::create([this]() { ThreadLoop(); });
}

BaseThread::~BaseThread()
{
    if (m_running)
    {
        Stop();
        m_impl->m_thread->wait();
    }

    delete m_impl;
}

void
BaseThread::Start(const char*, os::ThreadCore, ThreadPriority, uint32_t)
{
    m_impl->m_thread->start();
}

milliseconds
os::GetTimeStamp()
{
    static auto at_start = std::chrono::high_resolution_clock::now();

    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - at_start);
}

uint32_t
os::GetTimeStampRaw()
{
    return GetTimeStamp().count();
}

void
os::Sleep(milliseconds delay)
{
    QThread::msleep(delay.count());
}
