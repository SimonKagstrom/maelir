#include "menu_screen.hh"

#include "route_utils.hh"

#include <fmt/format.h>

UserInterface::MenuScreen::MenuScreen(UserInterface& parent, std::function<void()> on_close)
    : m_parent(parent)
    , m_on_close(on_close)

{
    auto state = m_parent.m_application_state.CheckoutReadonly();

    // Create a style for the selected state
    m_style_selected = lv_style_t {};

    lv_style_init(&m_style_selected);
    lv_style_set_bg_opa(&m_style_selected, LV_OPA_COVER); // Ensure the background is fully opaque
    lv_style_set_radius(&m_style_selected, 10);

    m_input_group = lv_group_create();
    m_menu = lv_menu_create(m_screen);

    lv_obj_set_size(m_menu, hal::kDisplayWidth * 0.72f, hal::kDisplayHeight * 0.72f);
    lv_obj_center(m_menu);

    lv_menu_set_mode_root_back_button(m_menu, LV_MENU_ROOT_BACK_BUTTON_ENABLED);

    auto back_button = lv_menu_get_main_header_back_button(m_menu);
    lv_obj_set_style_bg_color(m_screen, lv_obj_get_style_bg_color(m_menu, 0), 0);

    // Apply the style to the back button when it is focused
    lv_obj_add_style(back_button, &m_style_selected, LV_STATE_FOCUSED);
    lv_group_add_obj(m_input_group, back_button);
    lv_group_focus_obj(back_button);
    lv_obj_add_state(back_button, LV_STATE_FOCUS_KEY);


    lv_obj_t* sub_page = lv_menu_page_create(m_menu, NULL);
    lv_obj_t* main_page = lv_menu_page_create(m_menu, NULL);

    // TODO: If a home position is set
    AddEntry(main_page, "Navigate home", [this](auto) {
        auto state = m_parent.m_application_state.Checkout();
        state->demo_mode = false;

        m_parent.m_route_service.RequestRoute(
            m_parent.m_position,
            LandIndexToPoint(state->home_position, m_parent.m_land_mask_row_size));
        m_on_close();
    });
    AddEntry(main_page, "New route", [this](auto) {
        // Disable demo mode in this case
        m_parent.m_application_state.Checkout()->demo_mode = false;

        m_parent.SelectPosition(PositionSelection::kNewRoute);
        m_on_close();
    });
    AddEntryToSubPage(main_page, "Recall route", sub_page);

    for (auto i = 0u; i < state->stored_positions.size(); i++)
    {
        auto point = LandIndexToPoint(state->stored_positions[i], m_parent.m_land_mask_row_size);

        AddEntry(
            sub_page,
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
    AddBooleanEntry(main_page, "Show speedometer", state->show_speedometer, [this](auto) {
        auto state = m_parent.m_application_state.Checkout();
        state->show_speedometer = !state->show_speedometer;
    });

    AddBooleanEntry(main_page, "Demo mode", state->demo_mode, [this](auto) {
        auto state = m_parent.m_application_state.Checkout();
        state->demo_mode = !state->demo_mode;
    });

    m_event_listeners.push_back(LvEventListener::Create(m_menu, LV_EVENT_CLICKED, [this](auto e) {
        auto obj = static_cast<lv_obj_t*>(lv_event_get_target(e));
        if (lv_menu_back_button_is_root(m_menu, obj))
        {
            m_on_close();
        }
    }));

    lv_menu_set_page(m_menu, main_page);
    lv_indev_set_group(m_parent.m_lvgl_input_dev, m_input_group);
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

    m_event_listeners.push_back(LvEventListener::Create(cont, LV_EVENT_CLICKED, on_click));

    return cont;
}

lv_obj_t*
UserInterface::MenuScreen::AddEntryToSubPage(lv_obj_t* page, const char* text, lv_obj_t* sub_page)
{
    auto cont = lv_menu_cont_create(page);
    auto label = lv_label_create(cont);

    lv_label_set_text(label, text);
    lv_group_add_obj(m_input_group, cont);
    lv_obj_add_style(cont, &m_style_selected, LV_STATE_FOCUSED);

    lv_menu_set_load_page_event(m_menu, cont, sub_page);

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
