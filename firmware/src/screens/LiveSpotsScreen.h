#pragma once
#include <functional>
#include <lvgl.h>
#include "../models/Page.h"
#include "../services/ClockService.h"
#include "../services/LiveSpotsService.h"
#include "../ui/HeaderBar.h"

class LiveSpotsScreen
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
    lv_obj_t* summaryLabel = nullptr;
    lv_obj_t* recentLabel = nullptr;
    lv_obj_t* statusLabel = nullptr;
    lv_obj_t* titleLabel = nullptr;
    lv_timer_t* updateTimer = nullptr;
};
