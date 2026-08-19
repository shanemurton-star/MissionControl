#pragma once

#include <functional>
#include <lvgl.h>

#include "../models/Page.h"
#include "../services/ClockService.h"
#include "../services/PotaService.h"
#include "../ui/HeaderBar.h"

class PotaScreen
{
public:
    using NavigationCallback = std::function<void(Page)>;
    void begin(ClockService& clockService, PotaService& potaService);
    void show();
    void setNavigationCallback(NavigationCallback callback);

private:
    static void backButtonEventHandler(lv_event_t* event);
    static void updateTimerCallback(lv_timer_t* timer);
    void update();

    HeaderBar headerBar;
    PotaService* potaService = nullptr;
    NavigationCallback navigationCallback;
    lv_obj_t* screen = nullptr;
    lv_obj_t* statusLabel = nullptr;
    lv_obj_t* columnLabels[2] = {};
    lv_timer_t* updateTimer = nullptr;
};
