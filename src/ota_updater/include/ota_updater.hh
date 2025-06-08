#pragma once

#include "application_state.hh"
#include "base_thread.hh"
#include "hal/i_ota_updater.hh"

class OtaUpdater : public os::BaseThread
{
public:
    OtaUpdater(hal::IOtaUpdater& updater, ApplicationState& application_state);

private:
    std::optional<milliseconds> OnActivation() final;

    hal::IOtaUpdater& m_updater;
    ApplicationState& m_application_state;
    bool m_doing_update {false};

    std::unique_ptr<ApplicationState::IListener> m_state_listener;
};
