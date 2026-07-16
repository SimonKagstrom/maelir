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

    std::optional<milliseconds> OnActivation() final;

    hal::INvm& m_nvm;
    ApplicationState& m_application_state;

    std::unique_ptr<ListenerCookie> m_state_listener;
    std::unique_ptr<IRouteListener> m_route_listener;

    ApplicationState::PartialReadOnlyCache<AS::configuration, AS::stored_positions> m_state_cache;
};
