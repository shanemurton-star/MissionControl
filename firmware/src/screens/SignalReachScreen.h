#pragma once

#include <functional>
#include <lvgl.h>

#include "../models/Page.h"
#include "../services/ClockService.h"
#include "../services/LiveSpotsService.h"
#include "../ui/HeaderBar.h"

class SignalReachScreen
{
public:
    using NavigationCallback = std::function<void(Page)>;

    void begin(ClockService& clockService, LiveSpotsService& spotsService);
    void show();
    void release();
    void setNavigationCallback(NavigationCallback callback);

private:
    static void backButtonEventHandler(lv_event_t* event);
    static void updateTimerCallback(lv_timer_t* timer);
    void update();

    HeaderBar headerBar;
    LiveSpotsService* service = nullptr;
    NavigationCallback navigationCallback;
    lv_obj_t* screen = nullptr;
    lv_obj_t* titleLabel = nullptr;
    lv_obj_t* statusLabel = nullptr;
    lv_obj_t* homeMarker = nullptr;
    lv_obj_t* receiverCountLabel = nullptr;
    lv_obj_t* reportCountLabel = nullptr;
    lv_obj_t* farthestLabel = nullptr;
    lv_obj_t* regionsLabel = nullptr;
    lv_obj_t* emptyLabel = nullptr;
    lv_obj_t* refreshLabel = nullptr;
    lv_obj_t* signalDots[LiveSpotsService::MAX_SIGNAL_SPOTS] = {};
    uint32_t renderedDataRevision = UINT32_MAX;
    uint32_t renderedAgeMinute = UINT32_MAX;
    lv_timer_t* updateTimer = nullptr;
};
