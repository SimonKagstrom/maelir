#pragma once

#include "lv_event_listener.hh"
#include "ui.hh"

#include <functional>
#include <vector>

class UserInterface::MenuScreen : public ScreenBase
{
public:
    MenuScreen(UserInterface& parent, std::function<void()> on_close);

    ~MenuScreen();

    void Update() final;

private:
    UserInterface& m_parent;
    std::function<void()> m_on_close;
    lv_obj_t* m_menu;
    lv_group_t* m_input_group;

    std::vector<std::unique_ptr<LvEventListener>> m_event_listeners;
};
