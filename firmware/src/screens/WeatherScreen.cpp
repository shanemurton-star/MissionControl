#include "WeatherScreen.h"

#include "../models/Page.h"
#include "../ui/Theme.h"

void WeatherScreen::begin(
    ClockService& clockServiceReference,
    WeatherService& weatherServiceReference)
{
    if (screen != nullptr)
    {
        return;
    }

    clockService = &clockServiceReference;
    weatherService = &weatherServiceReference;

    screen = lv_obj_create(nullptr);

    Theme::configureScreen(screen);

    //
    // Header
    //
    lv_obj_t* header = lv_obj_create(screen);

    lv_obj_set_pos(
        header,
        0,
        0);

    lv_obj_set_size(
        header,
        Theme::SCREEN_WIDTH,
        Theme::HEADER_HEIGHT);

    Theme::configureHeader(header);

    lv_obj_t* title =
        Theme::createLabel(
            header,
            "MISSION CONTROL",
            Theme::COLOR_TEXT,
            &lv_font_montserrat_14);

    lv_obj_align(
        title,
        LV_ALIGN_LEFT_MID,
        12,
        0);

    //
    // Weather page title
    //
    lv_obj_t* pageTitle =
        Theme::createLabel(
            screen,
            "WEATHER",
            Theme::COLOR_PRIMARY,
            &lv_font_montserrat_14);

    lv_obj_align(
        pageTitle,
        LV_ALIGN_TOP_MID,
        0,
        90);

    //
    // Placeholder
    //
    lv_obj_t* message =
        Theme::createLabel(
            screen,
            "Weather page under construction",
            Theme::COLOR_TEXT_MUTED,
            &lv_font_montserrat_14);

    lv_obj_align(
        message,
        LV_ALIGN_CENTER,
        0,
        0);

    //
    // Bottom navigation
    //
    navigationBar.create(screen);
    navigationBar.setSelected(Page::Weather);
}

void WeatherScreen::show()
{
    if (screen == nullptr)
    {
        return;
    }

    lv_scr_load(screen);
}