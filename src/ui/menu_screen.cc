#include "menu_screen.hh"

UserInterface::MenuScreen::MenuScreen(UserInterface& parent, std::function<void()> on_close)
    : m_parent(parent)
    , m_on_close(on_close)

{
    m_menu = lv_menu_create(m_screen);


    lv_color_t bg_color = lv_obj_get_style_bg_color(m_menu, 0);
    if(lv_color_brightness(bg_color) > 127) {
        lv_obj_set_style_bg_color(m_menu, lv_color_darken(lv_obj_get_style_bg_color(m_menu, 0), 10), 0);
    }
    else {
        lv_obj_set_style_bg_color(m_menu, lv_color_darken(lv_obj_get_style_bg_color(m_menu, 0), 50), 0);
    }

    m_input_group = lv_group_create();

    lv_menu_set_mode_root_back_button(m_menu, LV_MENU_ROOT_BACK_BUTTON_ENABLED);
    lv_obj_add_event_cb(m_menu, BackEventHandler, LV_EVENT_CLICKED, this);

    auto back_button = lv_menu_get_main_header_back_button(m_menu);
    lv_group_add_obj(m_input_group, back_button);

    lv_obj_set_size(m_menu, hal::kDisplayWidth * 0.70f, hal::kDisplayHeight * 0.70f);
    lv_obj_center(m_menu);

    lv_obj_t * main_page = lv_menu_page_create(m_menu, NULL);

    auto cont = lv_menu_cont_create(main_page);
    auto label = lv_label_create(cont);
    lv_label_set_text(label, "Routing");
    lv_group_add_obj(m_input_group, cont);

    cont = lv_menu_cont_create(main_page);
    label = lv_label_create(cont);
    lv_label_set_text(label, "Show speedometer");
    lv_group_add_obj(m_input_group, cont);

    lv_menu_set_page(m_menu, main_page);

    lv_indev_set_group(m_parent.m_lvgl_input_dev, m_input_group);
    lv_group_add_obj(m_input_group, m_menu);
    lv_group_focus_next(m_input_group);
}

void
UserInterface::MenuScreen::Update()
{
}

void
UserInterface::MenuScreen::Activate()
{
    ScreenBase::Activate();
//    lv_group_add_obj(m_input_group, m_menu);
}

void
UserInterface::MenuScreen::Deactivate()
{
}


void
UserInterface::MenuScreen::BackEventHandler(lv_event_t* e)
{
    auto p = static_cast<MenuScreen*>(lv_event_get_user_data(e));
    p->m_on_close();
}
