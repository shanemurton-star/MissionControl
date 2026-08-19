#pragma once

#include <functional>
#include <lvgl.h>

#include "../services/ClockService.h"
#include "../models/AppSettings.h"

class HeaderBar
{
public:
    using SettingsCallback = std::function<void()>;

    static void configureSettings(const AppSettings& settings);

    void create(
        lv_obj_t* parent,
        ClockService& clockService,
        int16_t width,
        int16_t height);

    void update();
    void setSettingsCallback(SettingsCallback callback);

private:
    static void settingsEventHandler(lv_event_t* event);
    static const AppSettings* appSettings;

    ClockService* clockService = nullptr;

    lv_obj_t* container = nullptr;

    lv_obj_t* identityLabel = nullptr;
    lv_obj_t* wifiStatusLabel = nullptr;
    lv_obj_t* localTimeLabel = nullptr;
    lv_obj_t* dateLabel = nullptr;
    lv_obj_t* utcTimeLabel = nullptr;

    SettingsCallback settingsCallback;
};
