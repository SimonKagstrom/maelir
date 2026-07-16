#include "gps_reader.hh"

#include "position_converter.hh"

#include <cassert>
#include <etl/queue_spsc_atomic.h>
#include <span>

class GpsReader::GpsPortImpl final : public IGpsPort
{
public:
    GpsPortImpl(GpsReader* parent, uint8_t index)
        : m_parent(parent)
        , m_index(index)
    {
    }

    ~GpsPortImpl() final
    {
        // Mark this as stale
        m_parent->m_stale_listeners[m_index].store(true);
        m_parent->Awake();
    }

    void PushGpsData(const GpsData& data)
    {
        m_data.push(data);
        if (m_semaphore)
        {
            m_semaphore->release();
        }
    }

private:
    void DoAwakeOn(os::binary_semaphore* semaphore) final
    {
        m_semaphore = semaphore;
    }

    std::optional<GpsData> Poll() final
    {
        std::optional<GpsData> out = std::nullopt;
        GpsData data;

        // Just return the last data, history is not important
        while (m_data.pop(data))
        {
            out = data;
        }

        return out;
    }

    GpsReader* m_parent;
    etl::queue_spsc_atomic<GpsData, 8> m_data;
    os::binary_semaphore* m_semaphore {nullptr};
    const uint8_t m_index;
};


GpsReader::GpsReader(const MapMetadata& metadata,
                     ApplicationState& application_state,
                     hal::IGps& gps)
    : m_map_metadata(metadata)
    , m_application_state(application_state)
    , m_gps(gps)
{
}

std::unique_ptr<IGpsPort>
GpsReader::AttachListener()
{
    assert(!m_listeners.full());

    auto out = std::make_unique<GpsPortImpl>(this, m_listeners.size());
    m_listeners.push_back(out.get());

    return out;
}

std::optional<milliseconds>
GpsReader::OnActivation()
{
    auto data = m_gps.WaitForData(GetSemaphore());

    if (data->position)
    {
        m_position = data->position;
    }
    if (data->speed)
    {
        m_speed = data->speed;
    }
    if (data->heading)
    {
        m_heading = data->heading;
    }

    if (!m_position || !m_speed || !m_heading)
    {
        // Wait for the complete data
        return std::nullopt;
    }

    if (m_application_state.CheckoutReadonly().Get<AS::demo_mode>() == false)
    {
        auto conf = m_application_state.CheckoutReadonly().Get<AS::configuration>();
        auto qw =
            m_application_state
                .CheckoutQueuedWriter<AS::position, AS::pixel_position, AS::gps_position_valid>();

        GpsData mangled;

        mangled.position = *m_position;
        mangled.heading = *m_heading;
        mangled.speed = *m_speed;

        auto pixel_position = gps::PositionToPoint(m_map_metadata, *m_position);

        // Adjust the GPS data
        pixel_position.x += conf->longitude_adjustment;
        pixel_position.y += conf->latitude_adjustment;

        qw.Set<AS::position>(mangled);
        qw.Set<AS::pixel_position>(pixel_position);
        qw.Set<AS::gps_position_valid>(true);

        Reset();
    }

    return std::nullopt;
}


void
GpsReader::Reset()
{
    m_position = std::nullopt;
    m_speed = std::nullopt;
    m_heading = std::nullopt;
}
