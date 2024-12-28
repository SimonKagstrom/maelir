#include "base_thread.hh"

using namespace os;

BaseThread::BaseThread()
    : m_timer_manager(m_semaphore)
{
}

BaseThread::~BaseThread()
{
}

void
BaseThread::Start(uint8_t, ThreadPriority)
{
}

void
os::Sleep(milliseconds delay)
{
}
