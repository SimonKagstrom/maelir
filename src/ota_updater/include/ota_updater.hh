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

    bool ApplicationHasBeenUpdated() const;

    const char *GetInstructions() const;

private:
    std::optional<milliseconds> OnActivation() final;

    hal::IOtaUpdater& m_updater;
    ApplicationState& m_application_state;
    bool m_doing_update {false};
    const bool m_has_been_updated;

    std::unique_ptr<ApplicationState::IListener> m_state_listener;
    std::function<void(uint8_t)> m_progress;
    os::TimerHandle m_application_valid_timer;

    etl::mutex m_mutex;
    char m_instructions[80];
};
