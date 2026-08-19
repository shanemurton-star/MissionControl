#pragma once

#include <lvgl.h>

#include "../services/RadarService.h"

class WeatherRadarPanel
{
public:
    void create(
        lv_obj_t* parent,
        RadarService& radarService,
        int16_t x,
        int16_t y,
        int16_t width,
        int16_t height);

    void update();

private:
    void showBaseMapImage();
    void showRadarImage();
    void positionLocationOverlay();
    void updateStatusText();

    RadarService* radarService = nullptr;

    lv_obj_t* panel = nullptr;
    lv_obj_t* baseMapImage = nullptr;
    lv_obj_t* radarImage = nullptr;
    lv_obj_t* rangeRings[3] = {};
    lv_obj_t* locationMarker = nullptr;
    lv_obj_t* locationLabel = nullptr;
    lv_obj_t* statusLabel = nullptr;
    lv_obj_t* attributionLabel = nullptr;

    lv_img_dsc_t baseMapDescriptor = {};
    lv_img_dsc_t radarDescriptor = {};
    uint32_t displayedGeneration = 0;
};
