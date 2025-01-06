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

    lv_obj_set_size(m_menu, hal::kDisplayWidth * 0.70f, hal::kDisplayHeight * 0.70f);
    lv_obj_center(m_menu);

    lv_obj_t* main_page = lv_menu_page_create(m_menu, NULL);

    auto cont = lv_menu_cont_create(main_page);
    auto label = lv_label_create(cont);
    lv_label_set_text(label, "Routing");
    lv_group_add_obj(m_input_group, cont);

    cont = lv_menu_cont_create(main_page);
    label = lv_label_create(cont);
    lv_obj_set_flex_grow(label, 1);
    lv_label_set_text(label, "Speedometer");
    auto speedometer_switch = lv_switch_create(cont);
    lv_obj_add_state(speedometer_switch, state->show_speedometer ? LV_STATE_CHECKED : 0);

    lv_group_add_obj(m_input_group, speedometer_switch);

    lv_menu_set_page(m_menu, main_page);


    m_event_listeners.push_back(
        LvEventListener::Create(m_menu, LV_EVENT_CLICKED, [this]() { m_on_close(); }));
    m_event_listeners.push_back(
        LvEventListener::Create(speedometer_switch, LV_EVENT_CLICKED, [this]() {
            auto state = m_parent.m_application_state.Checkout();
            state->show_speedometer = !state->show_speedometer;
        }));

    lv_group_add_obj(m_input_group, m_menu);
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
