#include "menu_screen.hh"

#include "painter.hh"
#include "route_utils.hh"
#include "version.hh"

UserInterface::MenuScreen::MenuScreen(UserInterface& parent, std::function<void()> on_close)
    : m_parent(parent)
    , m_on_close(on_close)

{
    m_thumbnail_buffer =
        std::make_unique<uint8_t[]>((kTileSize / 3) * (kTileSize / 3) * sizeof(uint16_t) *
                                    ApplicationState::kMaxStoredPositions);

    auto state = m_parent.m_application_state.CheckoutReadonly();

    // Create a style for the selected state
    m_style_selected = lv_style_t {};

    lv_style_init(&m_style_selected);
    lv_style_set_bg_opa(&m_style_selected, LV_OPA_COVER); // Ensure the background is fully opaque
    lv_style_set_radius(&m_style_selected, 10);

    m_input_group = lv_group_create();
    m_menu = lv_menu_create(m_screen);

    lv_obj_set_size(m_menu, hal::kDisplayWidth * 0.68f, hal::kDisplayHeight * 0.80f);
    lv_obj_center(m_menu);

    lv_menu_set_mode_root_back_button(m_menu, LV_MENU_ROOT_BACK_BUTTON_ENABLED);

    auto back_button = lv_menu_get_main_header_back_button(m_menu);
    lv_obj_set_style_bg_color(m_screen, lv_obj_get_style_bg_color(m_menu, 0), 0);

    // Apply the style to the back button when it is focused
    lv_obj_add_style(back_button, &m_style_selected, LV_STATE_FOCUSED);
    lv_group_add_obj(m_input_group, back_button);
    lv_group_focus_obj(back_button);
    lv_obj_add_state(back_button, LV_STATE_FOCUS_KEY);


    lv_obj_t* route_page = lv_menu_page_create(m_menu, NULL);
    lv_obj_t* settings_page = lv_menu_page_create(m_menu, NULL);
    lv_obj_t* color_mode_page = lv_menu_page_create(m_menu, NULL);
    lv_obj_t* main_page = lv_menu_page_create(m_menu, NULL);

    lv_obj_set_scrollbar_mode(main_page, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scrollbar_mode(route_page, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scrollbar_mode(settings_page, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scrollbar_mode(color_mode_page, LV_SCROLLBAR_MODE_OFF);


    // TODO: If a home position is set
    AddEntry(main_page, "Navigate home", [this](auto) {
        auto state = m_parent.m_application_state.Checkout();
        state->demo_mode = false;

        m_parent.m_route_service.RequestRoute(
            m_parent.m_position,
            LandIndexToPoint(state->home_position, m_parent.m_land_mask_row_size));
        m_on_close();
    });

    if (m_parent.m_route.empty())
    {
        AddEntry(main_page, "New route", [this](auto) {
            // Disable demo mode in this case
            m_parent.m_application_state.Checkout()->demo_mode = false;

            m_parent.SelectPosition(PositionSelection::kNewRoute);
            m_on_close();
        });
    }
    else
    {
        AddEntry(main_page, "Cancel route", [this](auto) {
            m_parent.m_application_state.Checkout()->demo_mode = false;

            m_parent.m_route.clear();
            m_on_close();
        });
    }
    AddEntryToSubPage(main_page, "Recall route", route_page);

    for (auto i = 0u; i < state->stored_positions.size(); i++)
    {
        auto point = LandIndexToPoint(state->stored_positions[i], m_parent.m_land_mask_row_size);
        auto buffer =
            m_thumbnail_buffer.get() + i * (kTileSize / 3) * (kTileSize / 3) * sizeof(uint16_t);

        AddMapEntry(
            route_page,
            point,
            buffer,
            "Route to " + std::to_string(point.x) + "," + std::to_string(point.y),
            [this, i](auto) {
                auto state = m_parent.m_application_state.Checkout();
                state->demo_mode = false;

                m_parent.m_route_service.RequestRoute(
                    m_parent.m_position,
                    LandIndexToPoint(state->stored_positions[i], m_parent.m_land_mask_row_size));
                m_on_close();
            });
    }

    AddSeparator(main_page);

    AddEntry(main_page, "Set home position", [this](auto) {
        m_parent.SelectPosition(PositionSelection::kHome);
        m_on_close();
    });
    AddSeparator(main_page);
    AddEntryToSubPage(main_page, "Settings", settings_page);

    AddEntryToSubPage(settings_page, "Map color mode", color_mode_page);

    auto on_color_mode = [this](auto wanted) {
        auto state = m_parent.m_application_state.Checkout();

        state->color_mode = wanted;

        m_on_close();
    };

    AddEntry(color_mode_page, "Color", [on_color_mode](auto) {
        on_color_mode(ApplicationState::ColorMode::kColor);
    });
    AddEntry(color_mode_page, "Black/white", [on_color_mode](auto) {
        on_color_mode(ApplicationState::ColorMode::kBlackWhite);
    });
    AddEntry(color_mode_page, "Black/white + red", [on_color_mode](auto) {
        on_color_mode(ApplicationState::ColorMode::kBlackRed);
    });

    AddBooleanEntry(settings_page, "Show speedometer", state->show_speedometer, [this](auto) {
        auto state = m_parent.m_application_state.Checkout();
        state->show_speedometer = !state->show_speedometer;
    });

    AddBooleanEntry(settings_page, "Demo mode", state->demo_mode, [this](auto) {
        auto state = m_parent.m_application_state.Checkout();
        state->demo_mode = !state->demo_mode;
    });
    AddSeparator(settings_page);

    AddEntry(settings_page, "Adjust GPS", [this](auto) {
        auto state = m_parent.m_application_state.Checkout();
        state->demo_mode = false;

        m_parent.m_adjust_gps = true;
        m_on_close();
    });

    AddEntry(settings_page, std::format("OTA update ({})", kSoftwareVersion), [this](auto) {
        auto state = m_parent.m_application_state.Checkout();
        state->ota_update_active = true;

        // Disable, to avoid cache conflicts
        state->demo_mode = false;
        m_parent.EnterOtaUpdatingScreen();
        m_on_close();
    });

    m_event_listeners.push_back(LvEventListener::Create(m_menu, LV_EVENT_RELEASED, [this](auto e) {
        auto obj = static_cast<lv_obj_t*>(lv_event_get_target(e));
        if (lv_menu_back_button_is_root(m_menu, obj))
        {
            m_on_close();
        }
    }));

    lv_menu_set_page(m_menu, main_page);
    lv_indev_set_group(m_parent.m_lvgl_input_dev, m_input_group);

    Update();
}

UserInterface::MenuScreen::~MenuScreen()
{
    //    assert(lv_screen_active() != m_screen);

    if (lv_indev_get_group(m_parent.m_lvgl_input_dev) == m_input_group)
    {
        lv_indev_set_group(m_parent.m_lvgl_input_dev, nullptr);
    }
    lv_group_delete(m_input_group);
}

void
UserInterface::MenuScreen::Update()
{
    m_exit_timer = m_parent.StartTimer(10s, [this]() {
        m_on_close();
        return std::nullopt;
    });
}


lv_obj_t*
UserInterface::MenuScreen::AddEntry(lv_obj_t* page,
                                    const std::string& text,
                                    std::function<void(lv_event_t*)> on_click)
{
    auto cont = lv_menu_cont_create(page);
    auto label = lv_label_create(cont);

    lv_label_set_text(label, text.c_str());
    lv_group_add_obj(m_input_group, cont);
    lv_obj_add_style(cont, &m_style_selected, LV_STATE_FOCUSED);
    lv_obj_set_flex_grow(label, 1);

    m_event_listeners.push_back(LvEventListener::Create(cont, LV_EVENT_CLICKED, on_click));

    return cont;
}

lv_obj_t*
UserInterface::MenuScreen::AddMapEntry(lv_obj_t* page,
                                       const Point& point,
                                       uint8_t* buffer,
                                       const std::string& text,
                                       std::function<void(lv_event_t*)> on_click)
{
    auto obj = AddEntry(page, text, on_click);
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

        lv_obj_t* thumbnail = lv_image_create(obj);
        lv_image_set_src(thumbnail, &m_thumbnails.back().lv_image_dsc);
    }

    return obj;
}

lv_obj_t*
UserInterface::MenuScreen::AddEntryToSubPage(lv_obj_t* page, const char* text, lv_obj_t* route_page)
{
    auto cont = lv_menu_cont_create(page);
    auto label = lv_label_create(cont);

    lv_label_set_text(label, text);
    lv_group_add_obj(m_input_group, cont);
    lv_obj_add_style(cont, &m_style_selected, LV_STATE_FOCUSED);

    lv_menu_set_load_page_event(m_menu, cont, route_page);

    return cont;
}

void
UserInterface::MenuScreen::AddBooleanEntry(lv_obj_t* page,
                                           const char* text,
                                           bool default_value,
                                           std::function<void(lv_event_t*)> on_click)
{
    auto selected_switch_color = lv_palette_main(LV_PALETTE_LIGHT_GREEN);

    auto cont = lv_menu_cont_create(page);
    auto label = lv_label_create(cont);

    lv_obj_add_style(cont, &m_style_selected, LV_STATE_FOCUSED);
    lv_obj_set_flex_grow(label, 1);
    lv_label_set_text(label, text);

    auto boolean_switch = lv_switch_create(cont);
    lv_obj_add_state(boolean_switch, default_value ? LV_STATE_CHECKED : 0);
    // Highlight the label as well
    lv_obj_add_flag(boolean_switch, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_style_bg_color(
        boolean_switch, selected_switch_color, (int)LV_PART_INDICATOR | (int)LV_STATE_CHECKED);

    lv_group_add_obj(m_input_group, boolean_switch);

    m_event_listeners.push_back(
        LvEventListener::Create(boolean_switch, LV_EVENT_CLICKED, on_click));
}

void
UserInterface::MenuScreen::AddSeparator(lv_obj_t* page)
{
    auto separator_color = lv_palette_main(LV_PALETTE_GREY);

    auto cont = lv_menu_cont_create(page);

    // Create a style for the separator line
    static lv_style_t style_separator;
    lv_style_init(&style_separator);
    lv_style_set_bg_opa(&style_separator, LV_OPA_COVER); // Ensure the background is fully opaque
    lv_style_set_bg_color(&style_separator, separator_color); // Set the background color to black
    lv_style_set_height(&style_separator, 2);          // Set the height of the separator line
    lv_style_set_width(&style_separator, lv_pct(100)); // Set the height of the separator line

    // Create the separator and apply the style
    auto separator = lv_menu_separator_create(cont);
    lv_obj_add_style(separator, &style_separator, 0);
}
