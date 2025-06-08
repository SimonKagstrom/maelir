#include "ota_updater.hh"

#include <mutex>

OtaUpdater::OtaUpdater(hal::IOtaUpdater& updater, ApplicationState& application_state)
    : m_updater(updater)
    , m_application_state(application_state)
    , m_state_listener(application_state.AttachListener(GetSemaphore()))
    , m_progress([](auto) { /* Do nothing by default */ })
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
            m_updater.Setup([this](auto progress) {
                std::scoped_lock lock(m_mutex);
                m_progress(progress);
            });
        }
    }

    return out;
}

std::unique_ptr<ListenerCookie>
OtaUpdater::SubscribeOnProgress(const std::function<void(uint8_t)>& progress)
{
    std::scoped_lock lock(m_mutex);
    m_progress = progress;

    return std::make_unique<ListenerCookie>([this]() {
        std::scoped_lock release_lock(m_mutex);
        m_progress = [](auto) { /* nop! */ };
    });
}
