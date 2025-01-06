#include "application_state.hh"

#include <mutex>

class ApplicationState::ListenerImpl : public ApplicationState::IListener
{
public:
    ListenerImpl(ApplicationState& parent, os::binary_semaphore& semaphore)
        : m_parent(parent)
        , m_semaphore(semaphore)
    {
    }

    ~ListenerImpl() final
    {
        //std::erase(m_parent.m_listeners.begin(), m_parent.m_listeners.end(), this);
    }

    void Awake()
    {
        m_semaphore.release();
    }

private:
    ApplicationState& m_parent;
    os::binary_semaphore& m_semaphore;
};

struct ApplicationState::StateImpl : ApplicationState::State
{
    StateImpl(ApplicationState& parent)
        : m_parent(parent)
    {
        demo_mode = parent.m_global_state.demo_mode;
        gps_connected = parent.m_global_state.gps_connected;
        bluetooth_connected = parent.m_global_state.bluetooth_connected;
        show_speedometer = parent.m_global_state.show_speedometer;
    }

    ~StateImpl() final
    {
        m_parent.Commit(this);
    }

    ApplicationState& m_parent;
};


std::unique_ptr<ApplicationState::IListener>
ApplicationState::AttachListener(os::binary_semaphore& semaphore)
{
    auto out = std::make_unique<ListenerImpl>(*this, semaphore);

    m_listeners.push_back(out.get());

    return out;
}

std::unique_ptr<ApplicationState::State>
ApplicationState::Checkout()
{
    std::lock_guard lock(m_mutex);

    return std::make_unique<StateImpl>(*this);
}

const ApplicationState::State*
ApplicationState::CheckoutReadonly() const
{
    return &m_global_state;
}

void
ApplicationState::Commit(const ApplicationState::StateImpl* state)
{
    std::lock_guard lock(m_mutex);

    if (m_global_state == *state)
    {
        return;
    }

    m_global_state = *state;
    for (auto listener : m_listeners)
    {
        listener->Awake();
    }
}
