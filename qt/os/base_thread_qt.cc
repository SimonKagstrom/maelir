#include "base_thread.hh"

#include <QThread>

using namespace os;

struct BaseThread::Impl
{
    QThread* m_thread;
};

BaseThread::BaseThread(uint8_t)
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
BaseThread::Start()
{
    m_impl->m_thread->start();
}
