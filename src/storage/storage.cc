#include "storage.hh"

constexpr auto kHome = "H";

constexpr auto kRoute0 = "R0";
constexpr auto kRoute1 = "R1";
constexpr auto kRoute2 = "R2";
constexpr auto kRoute3 = "R3";

Storage::Storage(hal::INvm& nvm, ApplicationState& application_state)
    : m_nvm(nvm)
    , m_application_state(application_state)
    , m_state_listener(application_state.AttachListener(GetSemaphore()))
{
}

void
Storage::OnStartup()
{
    auto state = m_application_state.Checkout();

    if (auto home_position = m_nvm.Get<decltype(ApplicationState::State::home_position)>(kHome);
        home_position)
    {
        state->home_position = *home_position;
    }

    m_stored_state = *state;
}

std::optional<milliseconds>
Storage::OnActivation()
{
    auto current_state = m_application_state.CheckoutReadonly();
    auto schedule_commit = false;

    if (current_state->home_position != m_stored_state.home_position)
    {
        m_nvm.Set(kHome, current_state->home_position);
        schedule_commit = true;
    }

    if (schedule_commit)
    {
        m_nvm.Commit();
    }

    return std::nullopt;
}
