#include "HeaderBar.h"

#include <WiFi.h>
#include "Theme.h"

const AppSettings* HeaderBar::appSettings = nullptr;

namespace
{
    lv_obj_t* createLine(lv_obj_t* parent, int16_t x, int16_t y, int16_t width)
    {
        lv_obj_t* line = lv_obj_create(parent);
        lv_obj_set_pos(line, x, y);
        lv_obj_set_size(line, width, 3);
        lv_obj_set_style_bg_color(line, Theme::color(Theme::COLOR_TEXT), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(line, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(line, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(line, 2, LV_PART_MAIN);
        lv_obj_set_style_pad_all(line, 0, LV_PART_MAIN);
        lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE);
        return line;
    }
}

void HeaderBar::configureSettings(const AppSettings& settings)
{
    appSettings = &settings;
}

void HeaderBar::create(lv_obj_t* parent, ClockService& clockServiceReference,
                       int16_t width, int16_t height)
{
    clockService = &clockServiceReference;

    container = lv_obj_create(parent);
    lv_obj_set_pos(container, 0, 0);
    lv_obj_set_size(container, width, height);
    Theme::configureHeader(container);

    lv_obj_t* menuTarget = lv_obj_create(container);
    lv_obj_set_pos(menuTarget, 8, 5);
    lv_obj_set_size(menuTarget, 62, height - 10);
    lv_obj_set_style_bg_opa(menuTarget, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(menuTarget, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(menuTarget, 0, LV_PART_MAIN);
    lv_obj_add_flag(menuTarget, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(menuTarget, LV_OBJ_FLAG_SCROLLABLE);
    createLine(menuTarget, 15, 14, 30);
    createLine(menuTarget, 15, 24, 30);
    createLine(menuTarget, 15, 34, 30);
    lv_obj_add_event_cb(menuTarget, settingsEventHandler, LV_EVENT_CLICKED, this);

    identityLabel = Theme::createLabel(container, "-- - ----", Theme::COLOR_TEXT,
                                       &lv_font_montserrat_28);
    lv_obj_align(identityLabel, LV_ALIGN_CENTER, 0, 0);

    wifiStatusLabel = Theme::createLabel(container, LV_SYMBOL_WIFI,
                                         Theme::COLOR_WARNING, &lv_font_montserrat_28);
    lv_obj_align(wifiStatusLabel, LV_ALIGN_RIGHT_MID, -24, 0);

    lv_obj_t* wifiTarget = lv_obj_create(container);
    lv_obj_set_size(wifiTarget, 74, height);
    lv_obj_align(wifiTarget, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_opa(wifiTarget, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(wifiTarget, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(wifiTarget, 0, LV_PART_MAIN);
    lv_obj_add_flag(wifiTarget, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(wifiTarget, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(wifiTarget, settingsEventHandler, LV_EVENT_CLICKED, this);

    lv_obj_t* footer = lv_obj_create(parent);
    lv_obj_set_pos(footer, 0, Theme::FOOTER_TOP);
    lv_obj_set_size(footer, width, Theme::FOOTER_HEIGHT);
    Theme::configureHeader(footer);

    localTimeLabel = Theme::createLabel(footer, "--:--:-- LOCAL", Theme::COLOR_TEXT,
                                        &lv_font_montserrat_20);
    lv_obj_align(localTimeLabel, LV_ALIGN_LEFT_MID, 14, 0);
    dateLabel = Theme::createLabel(footer, "Waiting for time...", Theme::COLOR_TEXT,
                                   &lv_font_montserrat_20);
    lv_obj_align(dateLabel, LV_ALIGN_CENTER, 0, 0);
    utcTimeLabel = Theme::createLabel(footer, "--:--:-- UTC", Theme::COLOR_TEXT,
                                      &lv_font_montserrat_20);
    lv_obj_align(utcTimeLabel, LV_ALIGN_RIGHT_MID, -14, 0);

    update();
}

void HeaderBar::setSettingsCallback(SettingsCallback callback)
{
    settingsCallback = callback;
}

void HeaderBar::settingsEventHandler(lv_event_t* event)
{
    HeaderBar* self = static_cast<HeaderBar*>(lv_event_get_user_data(event));
    if (self != nullptr && self->settingsCallback != nullptr) self->settingsCallback();
}

void HeaderBar::update()
{
    if (clockService == nullptr || identityLabel == nullptr || wifiStatusLabel == nullptr ||
        localTimeLabel == nullptr || dateLabel == nullptr || utcTimeLabel == nullptr) return;

    if (appSettings != nullptr)
    {
        String identity = appSettings->callsign + " - " + appSettings->gridSquare;
        lv_label_set_text(identityLabel, identity.c_str());
    }

    String local = clockService->getLocalTime() + " LOCAL";
    String utc = clockService->getUTCTime() + " UTC";
    lv_label_set_text(localTimeLabel, local.c_str());
    lv_label_set_text(dateLabel, clockService->getDisplayDate().c_str());
    lv_label_set_text(utcTimeLabel, utc.c_str());

    lv_obj_set_style_text_color(
        wifiStatusLabel,
        Theme::color(WiFi.status() == WL_CONNECTED ? Theme::COLOR_SUCCESS : Theme::COLOR_WARNING),
        LV_PART_MAIN);
}
