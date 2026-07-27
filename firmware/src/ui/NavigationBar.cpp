#include "NavigationBar.h"
#include "Theme.h"

void NavigationBar::create(
    lv_obj_t* parent)
{
    container = lv_obj_create(parent);

    lv_obj_set_pos(
        container,
        0,
        Theme::SCREEN_HEIGHT -
            Theme::NAVIGATION_HEIGHT);

    lv_obj_set_size(
        container,
        Theme::SCREEN_WIDTH,
        Theme::NAVIGATION_HEIGHT);

    Theme::configureNavigationBar(container);

    dashboardButton =
        lv_btn_create(container);

    weatherButton =
        lv_btn_create(container);

    hamButton =
        lv_btn_create(container);

    systemButton =
        lv_btn_create(container);

    settingsButton =
        lv_btn_create(container);

    lv_obj_set_size(
        dashboardButton,
        150,
        40);

    lv_obj_set_size(
        weatherButton,
        150,
        40);

    lv_obj_set_size(
        hamButton,
        150,
        40);

    lv_obj_set_size(
        systemButton,
        150,
        40);

    lv_obj_set_size(
        settingsButton,
        150,
        40);

    lv_obj_align(
        dashboardButton,
        LV_ALIGN_LEFT_MID,
        8,
        0);

    lv_obj_align_to(
        weatherButton,
        dashboardButton,
        LV_ALIGN_OUT_RIGHT_MID,
        8,
        0);

    lv_obj_align_to(
        hamButton,
        weatherButton,
        LV_ALIGN_OUT_RIGHT_MID,
        8,
        0);

    lv_obj_align_to(
        systemButton,
        hamButton,
        LV_ALIGN_OUT_RIGHT_MID,
        8,
        0);

    lv_obj_align_to(
        settingsButton,
        systemButton,
        LV_ALIGN_OUT_RIGHT_MID,
        8,
        0);

    lv_obj_t* label;

    label = lv_label_create(dashboardButton);
    lv_label_set_text(
        label,
        "Dashboard");
    lv_obj_center(label);

    label = lv_label_create(weatherButton);
    lv_label_set_text(
        label,
        "Weather");
    lv_obj_center(label);

    label = lv_label_create(hamButton);
    lv_label_set_text(
        label,
        "Ham");
    lv_obj_center(label);

    label = lv_label_create(systemButton);
    lv_label_set_text(
        label,
        "System");
    lv_obj_center(label);

    label = lv_label_create(settingsButton);
    lv_label_set_text(
        label,
        "Settings");
    lv_obj_center(label);

    lv_obj_add_event_cb(
        dashboardButton,
        buttonEventHandler,
        LV_EVENT_CLICKED,
        this);

    lv_obj_add_event_cb(
        weatherButton,
        buttonEventHandler,
        LV_EVENT_CLICKED,
        this);

    lv_obj_add_event_cb(
        hamButton,
        buttonEventHandler,
        LV_EVENT_CLICKED,
        this);

    lv_obj_add_event_cb(
        systemButton,
        buttonEventHandler,
        LV_EVENT_CLICKED,
        this);

    lv_obj_add_event_cb(
        settingsButton,
        buttonEventHandler,
        LV_EVENT_CLICKED,
        this);

    setSelected(Page::Dashboard);
}

void NavigationBar::setSelected(
    Page page)
{
    styleButton(
        dashboardButton,
        page == Page::Dashboard);

    styleButton(
        weatherButton,
        page == Page::Weather);

    styleButton(
        hamButton,
        page == Page::Ham);

    styleButton(
        systemButton,
        page == Page::System);

    styleButton(
        settingsButton,
        page == Page::Settings);
}

void NavigationBar::setCallback(
    NavigationCallback newCallback)
{
    callback = newCallback;
}

void NavigationBar::buttonEventHandler(
    lv_event_t* event)
{
    NavigationBar* navigation =
        static_cast<NavigationBar*>(
            lv_event_get_user_data(event));

    if (navigation == nullptr ||
        navigation->callback == nullptr)
    {
        return;
    }

    lv_obj_t* button =
        lv_event_get_target(event);

    if (button == navigation->dashboardButton)
    {
        navigation->callback(
            Page::Dashboard);
    }
    else if (button == navigation->weatherButton)
    {
        navigation->callback(
            Page::Weather);
    }
    else if (button == navigation->hamButton)
    {
        navigation->callback(
            Page::Ham);
    }
    else if (button == navigation->systemButton)
    {
        navigation->callback(
            Page::System);
    }
    else if (button == navigation->settingsButton)
    {
        navigation->callback(
            Page::Settings);
    }
}

void NavigationBar::styleButton(
    lv_obj_t* button,
    bool selected)
{
    if (button == nullptr)
    {
        return;
    }

    lv_obj_set_style_radius(
        button,
        6,
        LV_PART_MAIN);

    lv_obj_set_style_border_width(
        button,
        1,
        LV_PART_MAIN);

    if (selected)
    {
        lv_obj_set_style_bg_color(
            button,
            Theme::color(
                Theme::COLOR_PRIMARY),
            LV_PART_MAIN);

        lv_obj_set_style_text_color(
            button,
            Theme::color(
                Theme::COLOR_BACKGROUND),
            LV_PART_MAIN);
    }
    else
    {
        lv_obj_set_style_bg_color(
            button,
            Theme::color(
                Theme::COLOR_PANEL),
            LV_PART_MAIN);

        lv_obj_set_style_text_color(
            button,
            Theme::color(
                Theme::COLOR_TEXT),
            LV_PART_MAIN);
    }
}