#include "main_menu_screen.hh"

#include "painter.hh"
#include "route_utils.hh"
#include "version.hh"

#include <array>
#include <string_view>

constexpr auto kColorModeOptions = std::to_array<std::string_view>({
    // Short, since the roller is as wide as the longest option
    "Color",
    "B/W",
    "B/W + red",
});
static_assert(kColorModeOptions.size() == static_cast<size_t>(ColorMode::kValueCount));
static_assert(std::to_underlying(ColorMode::kColor) == 0);
static_assert(std::to_underlying(ColorMode::kBlackWhite) == 1);
static_assert(std::to_underlying(ColorMode::kBlackRed) == 2);

UserInterface::MainMenuScreen::MainMenuScreen(UserInterface& parent,
                                              std::function<void()> on_close)
    : ScreenBase(parent)
    , m_on_close(on_close)
{
    m_thumbnail_buffer = std::make_unique<uint8_t[]>((kTileSize / 3) * (kTileSize / 3) *
                                                     sizeof(uint16_t) * kMaxStoredPositions);

    m_menu = std::make_unique<::MenuScreen>(
        m_parent.GetTimerManager(), m_screen, m_parent.m_lvgl_input_dev, m_on_close);

    auto& state = m_parent.m_application_state;
    auto& main = m_menu->GetMainPage();

    // TODO: If a home position is set
    main.AddEntry("Navigate home", [this]() {
        m_parent.m_application_state.Set<AS::demo_mode>(false);

        m_parent.m_route_service.RequestRoute(
            m_parent.m_position,
            LandIndexToPoint(m_parent.m_application_state.Get<AS::configuration>()->home_position,
                             m_parent.m_land_mask_row_size));
        m_on_close();
    });

    if (m_parent.m_route.empty())
    {
        main.AddEntry("New route", [this]() {
            // Disable demo mode in this case
            m_parent.m_application_state.Set<AS::demo_mode>(false);

            m_parent.SelectPosition(PositionSelection::kNewRoute);
            m_on_close();
        });
    }
    else
    {
        main.AddEntry("Cancel route", [this]() {
            m_parent.m_application_state.Set<AS::demo_mode>(false);

            m_parent.m_route.clear();
            m_on_close();
        });
    }

    auto& route_page = main.AddSubPage("Recall route");
    auto stored_positions = state.Get<AS::stored_positions>();
    for (auto i = 0u; i < stored_positions->positions.size(); i++)
    {
        auto point =
            LandIndexToPoint(stored_positions->positions[i], m_parent.m_land_mask_row_size);
        auto buffer =
            m_thumbnail_buffer.get() + i * (kTileSize / 3) * (kTileSize / 3) * sizeof(uint16_t);

        AddMapEntry(
            route_page,
            point,
            buffer,
            "Route to " + std::to_string(point.x) + "," + std::to_string(point.y),
            [this, i]() {
                m_parent.m_application_state.Set<AS::demo_mode>(false);
                auto stored_positions = m_parent.m_application_state.Get<AS::stored_positions>();

                m_parent.m_route_service.RequestRoute(
                    m_parent.m_position,
                    LandIndexToPoint(stored_positions->positions[i],
                                     m_parent.m_land_mask_row_size));
                m_on_close();
            });
    }

    main.AddSeparator();

    main.AddEntry("Set home position", [this]() {
        m_parent.SelectPosition(PositionSelection::kHome);
        m_on_close();
    });
    main.AddSeparator();

    auto& settings_page = main.AddSubPage("Settings");
    auto conf = state.Get<AS::configuration>();

    settings_page.AddRollerEntry(
        "Map colors",
        std::span<const std::string_view>(kColorModeOptions),
        kColorModeOptions[std::to_underlying(conf->color_mode)],
        [this](auto which) {
            auto ps = m_parent.m_application_state.CheckoutPartialSnapshot<AS::configuration>();
            ps.GetWritableReference<AS::configuration>().color_mode =
                static_cast<ColorMode>(which);
        });

    settings_page.AddBooleanEntry("Show speedometer", conf->show_speedometer, [this](auto value) {
        auto ps = m_parent.m_application_state.CheckoutPartialSnapshot<AS::configuration>();
        ps.GetWritableReference<AS::configuration>().show_speedometer = value;
    });

    settings_page.AddBooleanEntry("Demo mode", state.Get<AS::demo_mode>(), [this](auto value) {
        m_parent.m_application_state.Set<AS::demo_mode>(value);
    });
    settings_page.AddSeparator();

    settings_page.AddEntry("Adjust GPS", [this]() {
        m_parent.m_application_state.Set<AS::demo_mode>(false);

        m_parent.m_adjust_gps = true;
        m_on_close();
    });

    settings_page.AddEntry(std::format("OTA update ({})", kSoftwareVersion), [this]() {
        auto qw = m_parent.m_application_state
                      .CheckoutQueuedWriter<AS::ota_update_active, AS::demo_mode>();
        qw.Set<AS::ota_update_active>(true);

        // Disable, to avoid cache conflicts
        qw.Set<AS::demo_mode>(false);
        m_parent.EnterOtaUpdatingScreen();
        m_on_close();
    });
}

void
UserInterface::MainMenuScreen::OnInput(hal::IInput::Event event)
{
    m_menu->BumpExitTimer();
}

void
UserInterface::MainMenuScreen::AddMapEntry(::MenuScreen::Page& page,
                                           const Point& point,
                                           uint8_t* buffer,
                                           const std::string& text,
                                           const std::function<void()>& on_click)
{
    auto row = page.AddEntry(text, on_click);
    auto tile = m_parent.m_tile_producer.LockTile(point);
    if (tile)
    {
        auto p_16 = reinterpret_cast<uint16_t*>(buffer);
        auto x_offset = (point.x % kTileSize) / 3;
        auto y_offset = (point.y % kTileSize) / 3;

        painter::ZoomedBlit(p_16, kTileSize / 3, tile->GetImage(), 3, {0, 0});

        // Mark as green
        for (auto x = -2; x < 2; x++)
        {
            for (auto y = -2; y < 2; y++)
            {
                auto dst_y = y_offset + y;
                auto dst_x = x_offset + x;

                if (dst_y < 0 || dst_y >= kTileSize / 3 || dst_x < 0 || dst_x >= kTileSize / 3)
                {
                    continue;
                }

                // Orange in rgb565
                p_16[dst_y * (kTileSize / 3) + dst_x] = 0xFBE4;
            }
        }

        m_thumbnails.push_back(Image {
            std::span<const uint8_t> {buffer, (kTileSize / 3) * (kTileSize / 3) * sizeof(uint16_t)},
            kTileSize / 3,
            kTileSize / 3});

        lv_obj_t* thumbnail = lv_image_create(row);
        lv_image_set_src(thumbnail, &m_thumbnails.back().lv_image_dsc);
    }
}
