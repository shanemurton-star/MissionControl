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
    void release();
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
    lv_obj_t* activeListLabel = nullptr;
    lv_obj_t* mapViewport = nullptr;
    lv_obj_t* mapStatusLabel = nullptr;
    lv_obj_t* mapTileImages[PotaService::MAP_TILE_COUNT] = {};
    lv_img_dsc_t mapTileDescriptors[PotaService::MAP_TILE_COUNT] = {};
    lv_obj_t* plotMarkers[PotaService::MAX_SPOTS] = {};
    uint32_t renderedDataRevision = UINT32_MAX;
    uint32_t renderedMapGeneration = UINT32_MAX;
    lv_timer_t* updateTimer = nullptr;
};
