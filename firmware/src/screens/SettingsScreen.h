#pragma once

#include <functional>
#include <lvgl.h>

#include "../models/Page.h"
#include "../services/ClockService.h"
#include "../services/SettingsService.h"
#include "../services/WiFiService.h"
#include "../services/LiveSpotsService.h"
#include "../ui/HeaderBar.h"

class SettingsScreen
{
public:
    using NavigationCallback = std::function<void(Page)>;

    void begin(
        ClockService& clockService,
        SettingsService& settingsService,
        WiFiService& wifiService,
        LiveSpotsService& liveSpotsService);
    void show();
    void setNavigationCallback(NavigationCallback callback);

private:
    static void backButtonEventHandler(lv_event_t* event);
    static void saveButtonEventHandler(lv_event_t* event);
    static void locationButtonEventHandler(lv_event_t* event);
    static void gridButtonEventHandler(lv_event_t* event);
    static void callsignButtonEventHandler(lv_event_t* event);
    static void wifiScanButtonEventHandler(lv_event_t* event);
    static void wifiNetworkEventHandler(lv_event_t* event);
    static void wifiScanCloseEventHandler(lv_event_t* event);
    static void textAreaEventHandler(lv_event_t* event);
    static void keyboardButtonEventHandler(lv_event_t* event);
    static void updateTimerCallback(lv_timer_t* timer);
    static void restartTimerCallback(lv_timer_t* timer);

    void save();
    void saveLocation();
    void saveGridSquare();
    void saveCallsign();
    void updateStatus();
    void showKeyboard(lv_obj_t* textArea);
    void updateKeyboardKeys();
    void scanForWiFi();

    HeaderBar headerBar;
    SettingsService* settingsService = nullptr;
    WiFiService* wifiService = nullptr;
    LiveSpotsService* liveSpotsService = nullptr;
    NavigationCallback navigationCallback;

    lv_obj_t* screen = nullptr;
    lv_obj_t* ssidTextArea = nullptr;
    lv_obj_t* passwordTextArea = nullptr;
    lv_obj_t* postalCodeTextArea = nullptr;
    lv_obj_t* gridSquareTextArea = nullptr;
    lv_obj_t* callsignTextArea = nullptr;
    lv_obj_t* keyboard = nullptr;
    lv_obj_t* activeTextArea = nullptr;
    lv_obj_t* keyboardButtons[40] = {};
    lv_obj_t* keyboardLabels[40] = {};
    lv_obj_t* statusLabel = nullptr;
    lv_obj_t* wifiScanOverlay = nullptr;
    lv_obj_t* wifiScanStatusLabel = nullptr;
    lv_obj_t* wifiNetworkButtons[8] = {};
    lv_obj_t* wifiNetworkLabels[8] = {};
    String scannedSsids[8];
    lv_timer_t* updateTimer = nullptr;
    bool restartPending = false;
    bool uppercaseKeyboard = false;
    bool symbolKeyboard = false;
};
