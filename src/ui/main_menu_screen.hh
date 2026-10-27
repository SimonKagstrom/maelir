#pragma once

#include "application_state.hh"
#include "image.hh"
#include "menu_screen.hh"
#include "ui.hh"

#include <etl/vector.h>
#include <functional>
#include <memory>

class UserInterface::MainMenuScreen : public ScreenBase
{
public:
    MainMenuScreen(UserInterface& parent, std::function<void()> on_close);

    void OnInput(hal::IInput::Event event) final;

private:
    void AddMapEntry(::MenuScreen::Page& page,
                     const Point& where,
                     uint8_t* buffer,
                     const std::string& text,
                     const std::function<void()>& on_click);

    std::function<void()> m_on_close;

    // Before the menu, which shows them
    std::unique_ptr<uint8_t[]> m_thumbnail_buffer;
    etl::vector<Image, kMaxStoredPositions> m_thumbnails;

    std::unique_ptr<::MenuScreen> m_menu;
};
