#include "storage.hh"

#include <cstddef>
#include <ranges>
#include <string_view>

enum class Key
{
    kHome,
    kSpeedometer,
    kColorMode,
    kLatitudeAdjustment,
    kLongitudeAdjustment,
    kRoute0,
    kRoute1,
    kRoute2,
    kRoute3,

    kValueCount,
};

constexpr auto kKeyToString = std::array {
    std::pair {
        Key::kHome,
        "H",
    },
    std::pair {
        Key::kSpeedometer,
        "S",
    },
    std::pair {
        Key::kColorMode,
        "C",
    },
    std::pair {
        Key::kLatitudeAdjustment,
        "Y",
    },
    std::pair {
        Key::kLongitudeAdjustment,
        "X",
    },
    std::pair {
        Key::kRoute0,
        "0",
    },
    std::pair {
        Key::kRoute1,
        "1",
    },
    std::pair {
        Key::kRoute2,
        "2",
    },
    std::pair {
        Key::kRoute3,
        "3",
    },
};

static_assert(kKeyToString.size() == std::to_underlying(Key::kValueCount));

consteval bool
KeysAreUnique()
{
    auto cpy = kKeyToString;
    std::ranges::sort(cpy, [](const auto& a, const auto& b) { return a.second[0] < b.second[0]; });

    auto [begin, end] = std::ranges::unique(
        cpy, [](const auto& a, const auto& b) { return a.second[0] == b.second[0]; });

    return begin == end;
}
static_assert(KeysAreUnique(), "Keys must be unique in their string representation");

constexpr auto
KeyToString(Key key)
{
    return std::find_if(kKeyToString.begin(),
                        kKeyToString.end(),
                        [key](const auto& pair) { return pair.first == key; })
        ->second;
}

Storage::Storage(hal::INvm& nvm,
                 ApplicationState& application_state,
                 std::unique_ptr<IRouteListener> route_listener)
    : m_nvm(nvm)
    , m_application_state(application_state)
    , m_state_listener(
          application_state.AttachListener<AS::configuration, AS::stored_positions>(GetSemaphore()))
    , m_route_listener(std::move(route_listener))
    , m_state_cache(application_state)
{
    auto ps =
        m_application_state.CheckoutPartialSnapshot<AS::configuration, AS::stored_positions>();
    auto& conf = ps.GetWritableReference<AS::configuration>();
    auto& stored_positions = ps.GetWritableReference<AS::stored_positions>();

    conf.color_mode =
        m_nvm.Get<ColorMode>(KeyToString(Key::kColorMode)).value_or(ColorMode::kColor);

    conf.home_position = m_nvm.Get<IndexType>(KeyToString(Key::kHome)).value_or(0);
    conf.latitude_adjustment = m_nvm.Get<int8_t>(KeyToString(Key::kLatitudeAdjustment)).value_or(0);
    conf.longitude_adjustment = m_nvm.Get<int8_t>(KeyToString(Key::kLongitudeAdjustment)).value_or(0);
    conf.show_speedometer = m_nvm.Get<bool>(KeyToString(Key::kSpeedometer)).value_or(true);

    stored_positions.positions.clear();
    for (unsigned i = 0; i < kMaxStoredPositions; i++)
    {
        auto key = KeyToString(static_cast<Key>(std::to_underlying(Key::kRoute0) + i));
        auto position = m_nvm.Get<IndexType>(key);

        if (position)
        {
            stored_positions.positions.push_back(*position);
        }
    }

    m_route_listener->AwakeOn(GetSemaphore());
}


void
Storage::OnStartup()
{
    m_state_cache.Pull();
}

std::optional<milliseconds>
Storage::OnActivation()
{
    auto ro = m_application_state.CheckoutReadonly();
    auto& co = m_state_cache.Pull();


    if (co.IsChanged<AS::configuration>() || co.IsChanged<AS::stored_positions>())
    {
        printf("Configuration changed, writing to NVM...\n");
    }

    co.OnChangedValue<AS::configuration>([this](auto& old_conf, auto& new_conf) {
        if (old_conf.home_position != new_conf.home_position)
        {
            m_nvm.Set<IndexType>(KeyToString(Key::kHome), new_conf.home_position);
        }
        if (old_conf.latitude_adjustment != new_conf.latitude_adjustment)
        {
            m_nvm.Set<int8_t>(KeyToString(Key::kLatitudeAdjustment), new_conf.latitude_adjustment);
        }
        if (old_conf.longitude_adjustment != new_conf.longitude_adjustment)
        {
            m_nvm.Set<int8_t>(KeyToString(Key::kLongitudeAdjustment),
                              new_conf.longitude_adjustment);
        }
        if (old_conf.show_speedometer != new_conf.show_speedometer)
        {
            m_nvm.Set<bool>(KeyToString(Key::kSpeedometer), new_conf.show_speedometer);
        }
        if (old_conf.color_mode != new_conf.color_mode)
        {
            m_nvm.Set<ColorMode>(KeyToString(Key::kColorMode), new_conf.color_mode);
        }
    });

    co.OnNewValue<AS::stored_positions>([this](const auto& new_stored_positions) {
        for (unsigned i = 0; i < kMaxStoredPositions; i++)
        {
            auto key = KeyToString(static_cast<Key>(std::to_underlying(Key::kRoute0) + i));

            if (i < new_stored_positions.positions.size())
            {
                m_nvm.Set<IndexType>(key, new_stored_positions.positions[i]);
            }
        }
    });

    return std::nullopt;
}
