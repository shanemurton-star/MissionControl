#pragma once

#include <functional>
#include <lvgl.h>

#include "../services/ClockService.h"
#include "../models/AppSettings.h"
#include "../models/Page.h"

class HeaderBar
{
public:
    using SettingsCallback = std::function<void()>;
    using NavigationCallback = std::function<void(Page)>;

    static void configureSettings(const AppSettings& settings);

    void create(
        lv_obj_t* parent,
        ClockService& clockService,
        int16_t width,
        int16_t height);

    void update();
    void setSettingsCallback(SettingsCallback callback);
    void setNavigationCallback(NavigationCallback callback);

private:
    static constexpr uint8_t MENU_ITEM_COUNT = 7;
    static void settingsEventHandler(lv_event_t* event);
    static void menuEventHandler(lv_event_t* event);
    static void menuItemEventHandler(lv_event_t* event);
    static void menuOverlayEventHandler(lv_event_t* event);
    void createMenu(lv_obj_t* parent);
    void showMenu();
    void hideMenu();
    static const AppSettings* appSettings;

    ClockService* clockService = nullptr;

    lv_obj_t* container = nullptr;

    lv_obj_t* identityLabel = nullptr;
    lv_obj_t* wifiStatusLabel = nullptr;
    lv_obj_t* localTimeLabel = nullptr;
    lv_obj_t* dateLabel = nullptr;
    lv_obj_t* utcTimeLabel = nullptr;
    lv_obj_t* menuOverlay = nullptr;
    lv_obj_t* menuButtons[MENU_ITEM_COUNT] = {};

    SettingsCallback settingsCallback;
    NavigationCallback navigationCallback;
};
