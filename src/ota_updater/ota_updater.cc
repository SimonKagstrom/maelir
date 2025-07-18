#include "ota_updater.hh"

#include <mutex>

OtaUpdater::OtaUpdater(hal::IOtaUpdater& updater, ApplicationState& application_state)
    : m_updater(updater)
    , m_application_state(application_state)
    , m_has_been_updated(m_updater.ApplicationHasBeenUpdated())
    , m_state_listener(application_state.AttachListener(GetSemaphore()))
    , m_progress([](auto) { /* Do nothing by default */ })
{
    m_instructions =
        std::format("Connect to the Wifi AP {}\nand go to http://192.168.4.1", m_updater.GetSsid());


    if (m_has_been_updated)
    {
        // Wait 10s and then mark the application as valid if it has been updated
        m_application_valid_timer = StartTimer(10s, [this]() {
            m_updater.MarkApplicationAsValid();
            return std::nullopt;
        });
    }
}

const char*
OtaUpdater::GetInstructions() const
{
    return m_instructions.c_str();
}

bool
OtaUpdater::ApplicationHasBeenUpdated() const
{
    // Context: Probably another thread
    return m_has_been_updated;
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
            m_updater.Update([this](auto progress) {
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
