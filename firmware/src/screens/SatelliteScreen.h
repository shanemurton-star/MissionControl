#pragma once

#include <functional>
#include <lvgl.h>

#include "../models/Page.h"
#include "../services/ClockService.h"
#include "../services/SatelliteService.h"
#include "../ui/HeaderBar.h"

class SatelliteScreen
{
public:
    using NavigationCallback = std::function<void(Page)>;
    void begin(ClockService& clockService, SatelliteService& satelliteService);
    void show();
    void release();
    void setNavigationCallback(NavigationCallback callback);

private:
    static void backButtonEventHandler(lv_event_t* event);
    static void detailBackButtonEventHandler(lv_event_t* event);
    static void satelliteRowEventHandler(lv_event_t* event);
    static void updateTimerCallback(lv_timer_t* timer);
    void update();
    void updateDetail();
    void showSatelliteDetail(uint8_t satelliteIndex);

    lv_obj_t* screen = nullptr;
    lv_obj_t* statusLabel = nullptr;
    lv_obj_t* passScroller = nullptr;
    lv_obj_t* passRowButtons[SatelliteService::SATELLITE_COUNT] = {};
    lv_obj_t* passRowLabels[SatelliteService::SATELLITE_COUNT] = {};
    lv_obj_t* passRowSeparators[SatelliteService::SATELLITE_COUNT] = {};
    uint8_t passRowSatelliteIndices[SatelliteService::SATELLITE_COUNT] = {};
    lv_obj_t* targets[SatelliteService::SATELLITE_COUNT] = {};
    lv_obj_t* targetLabels[SatelliteService::SATELLITE_COUNT] = {};
    HeaderBar headerBar;
    HeaderBar detailHeaderBar;
    lv_obj_t* detailScreen = nullptr;
    lv_obj_t* detailTitleLabel = nullptr;
    lv_obj_t* detailIdentityLabel = nullptr;
    lv_obj_t* detailPositionLabel = nullptr;
    lv_obj_t* detailPassLabel = nullptr;
    lv_obj_t* detailRadioLabel = nullptr;
    uint32_t selectedCatalogNumber = 0;
    SatelliteService* satelliteService = nullptr;
    NavigationCallback navigationCallback;
    lv_timer_t* updateTimer = nullptr;
};
