#include "gps_reader.hh"

#include "gps_utils.hh"

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


GpsReader::GpsReader(const MapMetadata& metadata, hal::IGps& gps)
    : m_map_metadata(metadata)
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

    if (data.position)
    {
        m_position = data.position;
    }
    if (data.speed)
    {
        m_speed = data.speed;
    }
    if (data.heading)
    {
        m_heading = data.heading;
    }

    if (!m_position || !m_speed || !m_heading)
    {
        // Wait for the complete data
        return std::nullopt;
    }

    GpsData mangled;

    mangled.position = *m_position;
    mangled.heading = *m_heading;
    mangled.speed = *m_speed;
    mangled.pixel_position = gps::PositionToPoint(m_map_metadata, *m_position);

    for (auto i = 0u; i < m_stale_listeners.size(); i++)
    {
        if (m_stale_listeners[i].exchange(false))
        {
            m_listeners.erase(m_listeners.begin() + i);
        }
    }

    for (auto& l : m_listeners)
    {
        l->PushGpsData(mangled);
    }

    Reset();

    return std::nullopt;
}


void
GpsReader::Reset()
{
    m_position = std::nullopt;
    m_speed = std::nullopt;
    m_heading = std::nullopt;
}


namespace gps
{

Point
PositionToPoint(const MapMetadata& metadata, const GpsPosition& gps_data)
{
    auto longitude_offset = gps_data.longitude - metadata.corner_longitude;
    auto latitude_offset = metadata.corner_latitude - gps_data.latitude;

    int32_t x = longitude_offset * metadata.pixel_longitude_size;
    int32_t y = latitude_offset * metadata.pixel_latitude_size;

    if (longitude_offset < 0)
    {
        x = 0;
    }
    if (latitude_offset < 0)
    {
        y = 0;
    }

    if (x > kTileSize * metadata.tile_row_size)
    {
        x = kTileSize * metadata.tile_row_size;
    }
    if (y > kTileSize * metadata.tile_column_size)
    {
        y = kTileSize * metadata.tile_column_size;
    }

    return Point {x, y};
}

GpsPosition
PointToPosition(const MapMetadata& metadata, const Point& pixel_position)
{
    GpsPosition gps_position;

    double longitude_offset = static_cast<double>(pixel_position.x) / metadata.pixel_longitude_size;
    double latitude_offset = static_cast<double>(pixel_position.y) / metadata.pixel_latitude_size;

    gps_position.longitude = metadata.corner_longitude + longitude_offset;
    gps_position.latitude = metadata.corner_latitude - latitude_offset;

    return gps_position;
}

} // namespace gps
