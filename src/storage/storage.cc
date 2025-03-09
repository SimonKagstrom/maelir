#include "storage.hh"

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

void
Storage::OnStartup()
{
    auto state = m_application_state.Checkout();

    if (auto home_position = m_nvm.Get<decltype(ApplicationState::State::home_position)>(kHome);
        home_position)
    {
        state->home_position = *home_position;
    }

    if (auto color_mode = m_nvm.Get<decltype(ApplicationState::State::color_mode)>(kColorMode);
        color_mode)
    {
        state->color_mode = *color_mode;
    }

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

    if (current_state->home_position != m_stored_state.home_position)
    {
        m_nvm.Set(kHome, current_state->home_position);
        schedule_commit = true;
    }

    if (current_state->color_mode != m_stored_state.color_mode)
    {
        m_nvm.Set(kColorMode, current_state->color_mode);
        schedule_commit = true;
    }

    if (schedule_commit)
    {
        m_nvm.Commit();
    }

    // The stored state is now written to flash
    m_stored_state = *current_state;

    return std::nullopt;
}
