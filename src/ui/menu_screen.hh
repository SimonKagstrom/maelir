#pragma once

#include "application_state.hh"
#include "lv_event_listener.hh"
#include "ui.hh"

#include <etl/vector.h>
#include <functional>
#include <string>
#include <vector>

class UserInterface::MenuScreen : public ScreenBase
{
public:
    MenuScreen(UserInterface& parent, std::function<void()> on_close);

    ~MenuScreen();

    void OnInput(hal::IInput::Event event) final;

private:
    // Return the container object
    lv_obj_t*
    AddEntry(lv_obj_t* page, const std::string& text, std::function<void(lv_event_t*)> on_click);

    lv_obj_t* AddMapEntry(lv_obj_t* page,
                          const Point& where,
                          uint8_t* buffer,
                          const std::string& text,
                          std::function<void(lv_event_t*)> on_click);

    lv_obj_t* AddEntryToSubPage(lv_obj_t* page, const char* text, lv_obj_t* subpage);

    void AddSeparator(lv_obj_t* page);

    void AddBooleanEntry(lv_obj_t* page,
                         const char* text,
                         bool default_value,
                         std::function<void(lv_event_t*)> on_click);

    void BumpExitTimer();

    std::function<void()> m_on_close;

    lv_style_t m_style_selected;
    lv_obj_t* m_menu;
    lv_group_t* m_input_group;

    std::vector<std::unique_ptr<LvEventListener>> m_event_listeners;

    std::unique_ptr<uint8_t[]> m_thumbnail_buffer;
    etl::vector<Image, ApplicationState::kMaxStoredPositions> m_thumbnails;

    os::TimerHandle m_exit_timer;
};
