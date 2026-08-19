#pragma once
#include <functional>
#include <lvgl.h>
#include "../models/Page.h"
#include "../services/ClockService.h"
#include "../services/SolarService.h"
#include "../ui/HeaderBar.h"

class SolarScreen
{
public:
    using NavigationCallback = std::function<void(Page)>;
    void begin(ClockService& clockService, SolarService& solarService);
    void show();
    void setNavigationCallback(NavigationCallback callback);
private:
    static void backButtonEventHandler(lv_event_t* event);
    static void updateTimerCallback(lv_timer_t* timer);
    void update();
    HeaderBar headerBar;
    SolarService* solarService = nullptr;
    NavigationCallback navigationCallback;
    lv_obj_t* screen = nullptr;
    lv_obj_t* conditionLabel = nullptr;
    lv_obj_t* metricsLabel = nullptr;
    lv_obj_t* scalesLabel = nullptr;
    lv_obj_t* explanationLabel = nullptr;
    lv_obj_t* bandOutlookLabels[3] = {};
    lv_timer_t* updateTimer = nullptr;
};
