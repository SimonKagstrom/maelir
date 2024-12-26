#include "gps_reader.hh"

#include "gps_utils.hh"

#include <cassert>
#include <etl/queue_spsc_atomic.h>
#include <fmt/format.h>
#include <span>

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


namespace
{

auto
PositionSpanFromMetadata(const MapMetadata& metadata)
{
    const auto base = reinterpret_cast<const uint8_t*>(&metadata);
    return std::span<const MapGpsRasterTile> {
        reinterpret_cast<const MapGpsRasterTile*>(base + metadata.gps_position_offset),
        metadata.gps_data_rows * metadata.gps_data_row_size};
}

Point
MapGpsTileToPoint(const MapGpsRasterTile& tile, const GpsPosition& gps_data)
{
    float diff_longitude = tile.longitude_offset;
    float diff_latitude = tile.latitude_offset;

    auto out =
        Point {static_cast<int32_t>(((gps_data.longitude - tile.longitude) / diff_longitude) *
                                    kGpsPositionSize),
               static_cast<int32_t>(((gps_data.latitude - tile.latitude) / diff_latitude) *
                                    kGpsPositionSize)};

    return out;
}


std::optional<Point>
PositionIsInTile(const MapMetadata& metadata,
                 std::span<const MapGpsRasterTile> positions,
                 int32_t x,
                 int32_t y,
                 const GpsPosition& gps_data)
{
    uint32_t cur_index = y * metadata.gps_data_row_size + x;

    if (cur_index >= positions.size())
    {
        return std::nullopt;
    }

    // Make a copy to get it out of flash
    auto cur = positions[cur_index];

    if (gps_data.longitude >= cur.longitude &&
        gps_data.longitude <= cur.longitude + cur.longitude_offset &&
        gps_data.latitude <= cur.latitude &&
        gps_data.latitude >= cur.latitude + cur.latitude_offset)
    {
        float diff_longitude = cur.longitude_offset;
        float diff_latitude = cur.latitude_offset;

        auto out = Point {
            x * kGpsPositionSize +
                static_cast<int32_t>(((gps_data.longitude - cur.longitude) / diff_longitude) *
                                     kGpsPositionSize),
            y * kGpsPositionSize +
                static_cast<int32_t>(
                    ((gps_data.latitude - cur.latitude + cur.latitude_offset) / diff_latitude) *
                    kGpsPositionSize)};

//        fmt::print("Found in tile {} ({},{}) -> {},{}\n", cur_index, x, y, out.x, out.y);

        return out;
    }

    return std::nullopt;
}

} // namespace


namespace gps
{

Point
PositionToPoint(const MapMetadata& metadata, const GpsPosition& gps_data)
{
    if (gps_data.longitude < metadata.lowest_longitude ||
        gps_data.longitude > metadata.highest_longitude ||
        gps_data.latitude < metadata.lowest_latitude ||
        gps_data.latitude > metadata.highest_latitude)
    {
        return {0, 0};
    }

    auto positions = PositionSpanFromMetadata(metadata);

    float diff_latitude = metadata.highest_latitude - metadata.lowest_latitude;
    float diff_longitude = metadata.highest_longitude - metadata.lowest_longitude;

    int tile_x = ((gps_data.longitude - metadata.lowest_longitude) / diff_longitude) *
                 metadata.gps_data_row_size;
    int tile_y =
        ((metadata.highest_latitude - gps_data.latitude) / diff_latitude) * metadata.gps_data_rows;

    auto idx = tile_y * metadata.gps_data_row_size + tile_x;

    constexpr auto kScanDistance = 3;
    for (auto dx = -kScanDistance; dx < kScanDistance; dx++)
    {
        for (auto dy = -kScanDistance; dy < kScanDistance; dy++)
        {
            if (auto point =
                    PositionIsInTile(metadata, positions, tile_x + dx, tile_y + dy, gps_data);
                point)
            {
                return *point;
            }
        }
    }

    return {0, 0};
}

GpsPosition
PointToPosition(const MapMetadata& metadata, const Point& pixel_position)
{
    auto x = pixel_position.x;
    auto y = pixel_position.y;

    auto positions = PositionSpanFromMetadata(metadata);
    auto index = (y / kGpsPositionSize - 1) * metadata.gps_data_row_size + x / kGpsPositionSize;

    if (index >= positions.size())
    {
        fmt::print("BUt why?? {} vs {}\n", index, positions.size());
        // Should never happen
        return GpsPosition {.latitude = 0, .longitude = 0};
    }
    const auto& pos = positions[index];

    float x_offset = x % kGpsPositionSize;
    float y_offset = y % kGpsPositionSize;

    auto longitude = pos.longitude + (pos.longitude_offset * x_offset) / kGpsPositionSize;
    auto latitude = pos.latitude + (pos.latitude_offset * y_offset) / kGpsPositionSize;

//    fmt::print("PTP: idx {} ({}x{}), which is {},{}..{},{}. {},{} -> {},{}\n",
//               index,
//               index % metadata.gps_data_row_size,
//               index / metadata.gps_data_row_size,
//               pos.latitude,
//               pos.longitude,
//               pos.latitude + pos.latitude_offset,
//               pos.longitude + pos.longitude_offset,
//               x,
//               y,
//               longitude,
//               latitude);
    return GpsPosition {.latitude = latitude, .longitude = longitude};
}

} // namespace gps
