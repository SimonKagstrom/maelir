#include "gps_reader.hh"

#include <cassert>
#include <etl/queue_spsc_atomic.h>


class GpsReader::GpsPortImpl : public IGpsPort
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
    void AwakeOn(os::binary_semaphore& semaphore) final
    {
        m_semaphore = &semaphore;
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


GpsReader::GpsReader()
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
    for (auto i = 0u; i < m_stale_listeners.size(); i++)
    {
        if (m_stale_listeners[i].exchange(false))
        {
            m_listeners.erase(m_listeners.begin() + i);
        }
    }

    // TMP!
    for (auto& l : m_listeners)
    {
        l->PushGpsData({.latitude = 0.0, .longitude = 0.0, .speed = 13, .heading = 270});
    }
    return 100ms;
}
