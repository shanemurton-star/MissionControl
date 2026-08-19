#include "WeatherScreen.h"

#include "../ui/Theme.h"

void WeatherScreen::begin(
    ClockService& clockServiceReference,
    WeatherService& weatherServiceReference,
    RadarService& radarServiceReference)
{
    if (screen != nullptr)
    {
        return;
    }

    clockService = &clockServiceReference;
    weatherService = &weatherServiceReference;
    radarService = &radarServiceReference;

    screen = lv_obj_create(nullptr);

    Theme::configureScreen(screen);

    headerBar.create(
        screen,
        clockServiceReference,
        Theme::SCREEN_WIDTH,
        Theme::HEADER_HEIGHT);
    headerBar.setSettingsCallback([this]() {
        if (navigationCallback != nullptr) navigationCallback(Page::Settings);
    });

    lv_obj_t* backButton =
        lv_btn_create(screen);

    lv_obj_set_pos(
        backButton,
        8,
        Theme::CONTENT_TOP);

    lv_obj_set_size(
        backButton,
        116,
        36);

    lv_obj_set_style_bg_color(
        backButton,
        Theme::color(Theme::COLOR_PANEL),
        LV_PART_MAIN);

    lv_obj_set_style_border_color(
        backButton,
        Theme::color(Theme::COLOR_PANEL_BORDER),
        LV_PART_MAIN);

    lv_obj_set_style_border_width(
        backButton,
        1,
        LV_PART_MAIN);

    lv_obj_add_event_cb(
        backButton,
        backButtonEventHandler,
        LV_EVENT_CLICKED,
        this);

    lv_obj_t* backLabel =
        Theme::createLabel(
            backButton,
            LV_SYMBOL_LEFT " DASHBOARD",
            Theme::COLOR_PRIMARY);

    lv_obj_center(backLabel);

    lv_obj_t* pageTitle =
        Theme::createLabel(
            screen,
            "WEATHER DETAIL",
            Theme::COLOR_PRIMARY,
            &lv_font_montserrat_14);

    lv_obj_align(
        pageTitle,
        LV_ALIGN_TOP_LEFT,
        140,
        Theme::CONTENT_TOP + 10);

    radarPanel.create(
        screen,
        radarServiceReference,
        8,
        110,
        500,
        328);

    currentConditionsPanel.create(
        screen,
        weatherServiceReference,
        516,
        110,
        276,
        160);

    forecastPanel.create(
        screen,
        weatherServiceReference,
        516,
        278,
        276,
        160);

    update();

    updateTimer = lv_timer_create(
        updateTimerCallback,
        1000,
        this);

}

void WeatherScreen::show()
{
    if (screen == nullptr)
    {
        return;
    }

    lv_scr_load(screen);
    update();
}

void WeatherScreen::setNavigationCallback(
    NavigationCallback callback)
{
    navigationCallback = callback;
}

void WeatherScreen::update()
{
    headerBar.update();
    radarPanel.update();
    currentConditionsPanel.update();
    forecastPanel.update();
}

void WeatherScreen::backButtonEventHandler(
    lv_event_t* event)
{
    WeatherScreen* weatherScreen =
        static_cast<WeatherScreen*>(
            lv_event_get_user_data(event));

    if (weatherScreen == nullptr ||
        weatherScreen->navigationCallback == nullptr)
    {
        return;
    }

    weatherScreen->navigationCallback(
        Page::Dashboard);
}

void WeatherScreen::updateTimerCallback(
    lv_timer_t* timer)
{
    WeatherScreen* weatherScreen =
        static_cast<WeatherScreen*>(
            timer->user_data);

    if (weatherScreen == nullptr)
    {
        return;
    }

    weatherScreen->update();
}
