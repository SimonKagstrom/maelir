#pragma once

#include <functional>
#include <lvgl.h>
#include <memory>

class LvEventListener
{
public:
    LvEventListener() = delete;

    static std::unique_ptr<LvEventListener>
    Create(lv_obj_t* obj, lv_event_code_t event, std::function<void()> cb)
    {
        return std::unique_ptr<LvEventListener>(new LvEventListener(obj, event, cb));
    }

private:
    LvEventListener(lv_obj_t* obj, lv_event_code_t event, std::function<void()> cb)
        : m_cb(cb)
    {
        lv_obj_add_event_cb(obj, EventHandler, event, this);
    }

    static void EventHandler(lv_event_t* e)
    {
        auto p = reinterpret_cast<LvEventListener*>(lv_event_get_user_data(e));
        p->m_cb();
    }

    std::function<void()> m_cb;
};
