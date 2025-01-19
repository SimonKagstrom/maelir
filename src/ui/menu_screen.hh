#pragma once

#include "lv_event_listener.hh"
#include "ui.hh"

#include <functional>
#include <string>
#include <vector>

class UserInterface::MenuScreen : public ScreenBase
{
public:
    MenuScreen(UserInterface& parent, std::function<void()> on_close);

    ~MenuScreen();

    void Update() final;

private:
    // Return the container object
    lv_obj_t*
    AddEntry(lv_obj_t* page, const std::string& text, std::function<void(lv_event_t*)> on_click);

    lv_obj_t* AddEntryToSubPage(lv_obj_t* page, const char* text, lv_obj_t* subpage);

    void AddSeparator(lv_obj_t* page);

    void AddBooleanEntry(lv_obj_t* page,
                         const char* text,
                         bool default_value,
                         std::function<void(lv_event_t*)> on_click);

    UserInterface& m_parent;
    std::function<void()> m_on_close;

    lv_style_t m_style_selected;
    lv_obj_t* m_menu;
    lv_group_t* m_input_group;

    std::vector<std::unique_ptr<LvEventListener>> m_event_listeners;
};
