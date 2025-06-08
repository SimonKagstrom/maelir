#pragma once

#include "application_state.hh"
#include "base_thread.hh"
#include "hal/i_ota_updater.hh"
#include "listener_cookie.hh"

#include <etl/mutex.h>

class OtaUpdater : public os::BaseThread
{
public:
    OtaUpdater(hal::IOtaUpdater& updater, ApplicationState& application_state);

    std::unique_ptr<ListenerCookie>
    SubscribeOnProgress(const std::function<void(uint8_t)>& progress);

private:
    std::optional<milliseconds> OnActivation() final;

    hal::IOtaUpdater& m_updater;
    ApplicationState& m_application_state;
    bool m_doing_update {false};

    std::unique_ptr<ApplicationState::IListener> m_state_listener;
    std::function<void(uint8_t)> m_progress;


    etl::mutex m_mutex;
};
