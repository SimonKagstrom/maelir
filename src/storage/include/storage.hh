#pragma once

#include "application_state.hh"
#include "base_thread.hh"
#include "hal/i_nvm.hh"
#include "route_service.hh"

class Storage : public os::BaseThread
{
public:
    Storage(hal::INvm& nvm,
            ApplicationState& application_state,
            std::unique_ptr<IRouteListener> route_listener);

private:
    void OnStartup() final;

    // Read a value from NVM
    template <typename M>
    void UpdateFromNvm(ApplicationState::State* current_state,
                       const char* key,
                       M ApplicationState::State::* member);

    // Write back changes to the NVM
    template <typename M>
    bool WriteBack(const ApplicationState::State* current_state,
                   const char* key,
                   M ApplicationState::State::* member);

    void CommitState();

    std::optional<milliseconds> OnActivation() final;

    hal::INvm& m_nvm;
    ApplicationState& m_application_state;
    ApplicationState::State m_stored_state;

    std::unique_ptr<ApplicationState::IListener> m_state_listener;
    std::unique_ptr<IRouteListener> m_route_listener;

    os::TimerHandle m_commit_timer;
};
