#pragma once

#include <functional>
#include <lvgl.h>

#include "../models/Page.h"
#include "../services/ClockService.h"
#include "../services/SettingsService.h"
#include "../services/WsjtxService.h"
#include "../ui/HeaderBar.h"

class CallsignLookupScreen
{
public:
    using NavigationCallback = std::function<void(Page)>;
    void begin(
        ClockService& clockService,
        SettingsService& settingsService,
        WsjtxService& wsjtxService);
    void show();
    void release();
    void setNavigationCallback(NavigationCallback callback);

private:
    static void backButtonEventHandler(lv_event_t* event);
    static void updateTimerCallback(lv_timer_t* timer);
    void update();

    HeaderBar headerBar;
    SettingsService* settingsService = nullptr;
    WsjtxService* service = nullptr;
    NavigationCallback navigationCallback;
    lv_obj_t* screen = nullptr;
    lv_obj_t* listenerLabel = nullptr;
    lv_obj_t* callLabel = nullptr;
    lv_obj_t* stateLabel = nullptr;
    lv_obj_t* radioLabel = nullptr;
    lv_obj_t* pathLabel = nullptr;
    lv_obj_t* nameLabel = nullptr;
    lv_obj_t* locationLabel = nullptr;
    lv_obj_t* providerLabel = nullptr;
    lv_obj_t* lookupStatusLabel = nullptr;
    lv_obj_t* photoImage = nullptr;
    lv_obj_t* photoPlaceholderLabel = nullptr;
    lv_obj_t* flagImage = nullptr;
    lv_img_dsc_t photoDescriptor = {};
    lv_img_dsc_t flagDescriptor = {};
    lv_timer_t* updateTimer = nullptr;
    uint32_t renderedRevision = UINT32_MAX;
    uint32_t renderedPacketAge = UINT32_MAX;
    uint32_t renderedPhotoGeneration = UINT32_MAX;
    uint32_t renderedFlagGeneration = UINT32_MAX;
    bool sidePhotoLayout = false;
};
