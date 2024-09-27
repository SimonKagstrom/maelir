#include "gps_reader.hh"

#include "tile.hh"

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
    : m_gps(gps)
    , m_corner_latitude(metadata.corner_latitude)
    , m_corner_longitude(metadata.corner_longitude)
    , m_latitude_pixel_size(metadata.pixel_latitude_size)
    , m_longitude_pixel_size(metadata.pixel_longitude_size)
    , m_tile_columns(metadata.tile_column_size)
    , m_tile_rows(metadata.tile_row_size)
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
    GpsData mangled;

    mangled.position = data.position;
    mangled.pixel_position = PositionToPoint(data.position);

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

    return std::nullopt;
}


PixelPosition
GpsReader::PositionToPoint(const GpsPosition& gps_data) const
{
    auto longitude_offset = gps_data.longitude - m_corner_longitude;
    auto latitude_offset = m_corner_latitude - gps_data.latitude;

    int32_t x = longitude_offset * m_longitude_pixel_size;
    int32_t y = latitude_offset * m_latitude_pixel_size;

    if (longitude_offset < 0)
    {
        x = 0;
    }
    if (latitude_offset < 0)
    {
        y = 0;
    }

    if (x > kTileSize * m_tile_rows)
    {
        x = kTileSize * m_tile_rows;
    }
    if (y > kTileSize * m_tile_columns)
    {
        y = kTileSize * m_tile_columns;
    }

    return PixelPosition {x, y};
}
