#include "mock_time.hh"

#include <cassert>

namespace
{
std::weak_ptr<MockTime> g_mock_time;
}

TimeFixture::TimeFixture()
{
    g_mock_time = m_time;
}

TimeFixture::~TimeFixture()
{
    g_mock_time.reset();
}

void
TimeFixture::AdvanceTime(milliseconds time)
{
    m_time->AdvanceTime(time);
}

void
TimeFixture::SetTime(milliseconds time)
{
    m_time->SetTime(time);
}

milliseconds
os::GetTimeStamp()
{
    auto p = g_mock_time.lock();

    assert(p);
    return p->Now();
}
