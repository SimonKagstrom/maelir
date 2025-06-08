#pragma once

#include "application_state.hh"
#include "listener_cookie.hh"
#include "ui.hh"

#include <etl/vector.h>
#include <functional>
#include <string>
#include <vector>

class UserInterface::UpdatingScreen : public ScreenBase
{
public:
    explicit UpdatingScreen(UserInterface& parent);

    void Update() final;

private:
    lv_obj_t* m_label;

    std::unique_ptr<ListenerCookie> m_progress_cookie;
    std::atomic<uint8_t> m_progress {0};
};
