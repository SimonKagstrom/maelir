#include "base_thread.hh"

#include <QThread>

using namespace os;

struct BaseThread::Impl
{
    QThread* m_thread;
};

BaseThread::BaseThread()
{
    m_impl = new Impl;
    m_impl->m_thread = QThread::create([this]() { ThreadLoop(); });
}

BaseThread::~BaseThread()
{
    Stop();
    m_impl->m_thread->wait();

    delete m_impl;
}

void
BaseThread::Start(uint8_t)
{
    m_impl->m_thread->start();
}

milliseconds
os::GetTimeStamp()
{
    return std::chrono::duration_cast<milliseconds>(
        std::chrono::system_clock::now().time_since_epoch());
}

void
os::Sleep(milliseconds delay)
{
    QThread::msleep(delay.count());
}
