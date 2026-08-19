#pragma once

#include <functional>
#include <lvgl.h>

#include "../models/Page.h"
#include "../services/AircraftService.h"
#include "../services/ClockService.h"
#include "../ui/HeaderBar.h"

class AircraftScreen
{
public:
    using NavigationCallback = std::function<void(Page)>;
    void begin(ClockService& clockService, AircraftService& aircraftService);
    void show();
    void setNavigationCallback(NavigationCallback callback);

private:
    static void backButtonEventHandler(lv_event_t* event);
    static void detailBackButtonEventHandler(lv_event_t* event);
    static void aircraftRowEventHandler(lv_event_t* event);
    static void updateTimerCallback(lv_timer_t* timer);
    void update();
    void updateDetail();
    void showAircraftDetail(uint8_t index);

    lv_obj_t* screen = nullptr;
    lv_obj_t* countLabel = nullptr;
    lv_obj_t* statusLabel = nullptr;
    lv_obj_t* listLabel = nullptr;
    lv_obj_t* aircraftRowButtons[6] = {};
    lv_obj_t* aircraftRowLabels[6] = {};
    lv_obj_t* targets[AircraftService::MAX_AIRCRAFT] = {};
    lv_obj_t* targetLabels[AircraftService::MAX_AIRCRAFT] = {};
    HeaderBar headerBar;
    HeaderBar detailHeaderBar;
    lv_obj_t* detailScreen = nullptr;
    lv_obj_t* detailIdentityLabel = nullptr;
    lv_obj_t* detailFlightLabel = nullptr;
    lv_obj_t* detailPositionLabel = nullptr;
    String selectedAircraftHex;
    AircraftService* aircraftService = nullptr;
    NavigationCallback navigationCallback;
    lv_timer_t* updateTimer = nullptr;
};
