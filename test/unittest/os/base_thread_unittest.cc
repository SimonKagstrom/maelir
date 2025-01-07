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
BaseThread::Start(uint8_t, ThreadPriority, uint32_t)
{
}

void
os::Sleep(milliseconds delay)
{
}
