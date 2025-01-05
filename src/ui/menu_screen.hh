#pragma once

#include "ui.hh"

#include <functional>

class UserInterface::MenuScreen : public ScreenBase
{
public:
    MenuScreen(UserInterface& parent, std::function<void()> on_close);

    void Update() final;
    void Activate() final;
    void Deactivate() final;

private:
    static void BackEventHandler(lv_event_t* e);

    UserInterface& m_parent;
    std::function<void()> m_on_close;
    lv_obj_t* m_menu;
    lv_group_t* m_input_group;
};
