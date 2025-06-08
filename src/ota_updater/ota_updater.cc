#include "ota_updater.hh"

OtaUpdater::OtaUpdater(hal::IOtaUpdater& updater, ApplicationState& application_state)
    : m_updater(updater)
    , m_application_state(application_state)
    , m_state_listener(application_state.AttachListener(GetSemaphore()))
{
}

std::optional<milliseconds>
OtaUpdater::OnActivation()
{
    std::optional<milliseconds> out = std::nullopt;

    if (m_doing_update)
    {
        // Awake regularly while doing the update
        out = 10ms;
    }
    else
    {
        if (m_application_state.CheckoutReadonly()->ota_update_active)
        {
            m_doing_update = true;
            m_updater.Setup([](auto progress) {

            });
        }
    }

    return out;
}
