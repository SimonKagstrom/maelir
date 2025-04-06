#pragma once

#include <functional>

class ListenerCookie
{
public:
    ListenerCookie(std::function<void()> on_destruction)
        : m_on_destruction(std::move(on_destruction))
    {
    }

    ListenerCookie() = delete;
    ListenerCookie(const ListenerCookie&) = delete;
    ListenerCookie& operator=(const ListenerCookie&) = delete;
    ListenerCookie(ListenerCookie&&) = delete;

    ~ListenerCookie()
    {
        m_on_destruction();
    }

private:
    std::function<void()> m_on_destruction;
};
