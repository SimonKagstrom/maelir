#include "menu_screen.hh"

UserInterface::MenuScreen::MenuScreen(UserInterface& parent, std::function<void()> on_close)
    : m_parent(parent)
    , m_on_close(on_close)

{
    auto state = m_parent.m_application_state.CheckoutReadonly();

    m_input_group = lv_group_create();
    m_menu = lv_menu_create(m_screen);
    lv_menu_set_mode_root_back_button(m_menu, LV_MENU_ROOT_BACK_BUTTON_ENABLED);

    auto back_button = lv_menu_get_main_header_back_button(m_menu);
    lv_group_add_obj(m_input_group, back_button);
    lv_group_focus_obj(back_button);
    lv_obj_add_state(back_button, LV_STATE_FOCUS_KEY);

    lv_obj_set_size(m_menu, hal::kDisplayWidth * 0.70f, hal::kDisplayHeight * 0.70f);
    lv_obj_center(m_menu);

    lv_obj_t* sub_page = lv_menu_page_create(m_menu, NULL);

    auto cont = lv_menu_cont_create(sub_page);
    auto label = lv_label_create(cont);
    lv_group_add_obj(m_input_group, cont);
    lv_label_set_text(label, "not yet");


    lv_obj_t* main_page = lv_menu_page_create(m_menu, NULL);

    cont = lv_menu_cont_create(main_page);
    label = lv_label_create(cont);
    lv_label_set_text(label, "New route");
    lv_group_add_obj(m_input_group, cont);

    cont = lv_menu_cont_create(main_page);
    label = lv_label_create(cont);
    lv_label_set_text(label, "Recall route");
    lv_group_add_obj(m_input_group, cont);
    lv_menu_set_load_page_event(m_menu, cont, sub_page);


    auto selected_switch_color = lv_palette_main(LV_PALETTE_LIGHT_GREEN);

    cont = lv_menu_cont_create(main_page);
    label = lv_label_create(cont);
    lv_obj_set_flex_grow(label, 1);
    lv_label_set_text(label, "Show speedometer");
    auto speedometer_switch = lv_switch_create(cont);
    lv_obj_add_state(speedometer_switch, state->show_speedometer ? LV_STATE_CHECKED : 0);
    // Highlight the label as well
    lv_obj_add_flag(speedometer_switch, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_style_bg_color(
        speedometer_switch, selected_switch_color, (int)LV_PART_INDICATOR | (int)LV_STATE_CHECKED);

    lv_group_add_obj(m_input_group, speedometer_switch);

    cont = lv_menu_cont_create(main_page);
    label = lv_label_create(cont);
    lv_obj_set_flex_grow(label, 1);
    lv_label_set_text(label, "Demo mode");
    auto demo_switch = lv_switch_create(cont);
    lv_obj_set_style_bg_color(
        demo_switch, selected_switch_color, (int)LV_PART_INDICATOR | (int)LV_STATE_CHECKED);

    lv_obj_add_state(demo_switch, state->demo_mode ? LV_STATE_CHECKED : 0);
    lv_obj_add_flag(demo_switch, LV_OBJ_FLAG_EVENT_BUBBLE);

    lv_group_add_obj(m_input_group, demo_switch);

    lv_menu_set_page(m_menu, main_page);


    m_event_listeners.push_back(
        LvEventListener::Create(m_menu, LV_EVENT_CLICKED, [this]() { m_on_close(); }));
    m_event_listeners.push_back(
        LvEventListener::Create(speedometer_switch, LV_EVENT_CLICKED, [this]() {
            auto state = m_parent.m_application_state.Checkout();
            state->show_speedometer = !state->show_speedometer;
        }));
    m_event_listeners.push_back(LvEventListener::Create(demo_switch, LV_EVENT_CLICKED, [this]() {
        auto state = m_parent.m_application_state.Checkout();
        state->demo_mode = !state->demo_mode;
    }));

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
