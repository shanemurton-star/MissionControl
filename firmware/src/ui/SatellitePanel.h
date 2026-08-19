#pragma once

#include <functional>
#include <lvgl.h>

#include "../services/SatelliteService.h"

class SatellitePanel
{
public:
    using ClickCallback = std::function<void()>;
    void create(lv_obj_t* parent, SatelliteService& service,
                int16_t x, int16_t y, int16_t width, int16_t height);
    void update();
    void setClickCallback(ClickCallback callback);

private:
    static void panelEventHandler(lv_event_t* event);
    SatelliteService* service = nullptr;
    ClickCallback clickCallback;
    lv_obj_t* panel = nullptr;
    lv_obj_t* nameLabel = nullptr;
    lv_obj_t* countdownLabel = nullptr;
    lv_obj_t* detailsLabel = nullptr;
};
