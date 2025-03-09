#include "storage.hh"

#include <cstddef>

constexpr auto kHome = "H";
constexpr auto kColorMode = "C";

constexpr auto kRoutes = std::array {
    "R0",
    "R1",
    "R2",
    "R3",
};
static_assert(kRoutes.size() == ApplicationState::kMaxStoredPositions);

Storage::Storage(hal::INvm& nvm,
                 ApplicationState& application_state,
                 std::unique_ptr<IRouteListener> route_listener)
    : m_nvm(nvm)
    , m_application_state(application_state)
    , m_state_listener(application_state.AttachListener(GetSemaphore()))
    , m_route_listener(std::move(route_listener))
{
    m_route_listener->AwakeOn(GetSemaphore());
}

template <typename M>
void
Storage::UpdateFromNvm(ApplicationState::State* current_state,
                       const char* key,
                       M ApplicationState::State::* member)
{
    if (auto value = m_nvm.Get<typeof(current_state->*member)>(key); value)
    {
        current_state->*member = *value;
    }
}

template <typename M>
bool
Storage::WriteBack(const ApplicationState::State* current_state,
                   const char* key,
                   M ApplicationState::State::* member)
{
    if (current_state->*member != m_stored_state.*member)
    {
        m_nvm.Set(key, current_state->*member);
        return true;
    }

    return false;
}

void
Storage::OnStartup()
{
    auto state = m_application_state.Checkout();

    UpdateFromNvm(state.get(), kHome, &ApplicationState::State::home_position);
    UpdateFromNvm(state.get(), kColorMode, &ApplicationState::State::color_mode);

    // Read the stored routes
    for (auto i = 0u; i < kRoutes.size(); i++)
    {
        if (auto position =
                m_nvm.Get<decltype(ApplicationState::State::stored_positions)::value_type>(
                    kRoutes[i]);
            position)
        {
            state->stored_positions.push_back(*position);
        }
    }

    m_stored_state = *state;
}

std::optional<milliseconds>
Storage::OnActivation()
{
    auto current_state = m_application_state.Checkout();
    auto schedule_commit = false;

    std::optional<IndexType> new_route_destination;
    while (auto route = m_route_listener->Poll())
    {
        if (route->type == IRouteListener::EventType::kReady && route->route.size() > 1 &&
            current_state->demo_mode == false)
        {
            new_route_destination = route->route.back();
        }
    }

    if (new_route_destination)
    {
        auto it = std::find(current_state->stored_positions.begin(),
                            current_state->stored_positions.end(),
                            *new_route_destination);
        if (it == current_state->stored_positions.end())
        {
            if (current_state->stored_positions.full())
            {
                current_state->stored_positions.pop_back();
            }
            current_state->stored_positions.push_front(*new_route_destination);
        }
        else
        {
            current_state->stored_positions.erase(it);
            current_state->stored_positions.push_front(*new_route_destination);
        }

        for (unsigned i = 0; i < current_state->stored_positions.size(); i++)
        {
            m_nvm.Set(kRoutes[i], current_state->stored_positions[i]);
        }

        schedule_commit = true;
    }

    schedule_commit |=
        WriteBack(current_state.get(), kHome, &ApplicationState::State::home_position);
    schedule_commit |=
        WriteBack(current_state.get(), kColorMode, &ApplicationState::State::color_mode);

    if (schedule_commit)
    {
        m_nvm.Commit();
    }

    // The stored state is now written to flash
    m_stored_state = *current_state;

    return std::nullopt;
}
