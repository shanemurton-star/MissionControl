#pragma once

#include <functional>
#include <lvgl.h>

#include "../services/AircraftService.h"

class AircraftPanel
{
public:
    using ClickCallback = std::function<void()>;

    void create(lv_obj_t* parent, AircraftService& service,
                int16_t x, int16_t y, int16_t width, int16_t height);
    void update();
    void setClickCallback(ClickCallback callback);

private:
    static void panelEventHandler(lv_event_t* event);

    AircraftService* service = nullptr;
    ClickCallback clickCallback;
    lv_obj_t* panel = nullptr;
    lv_obj_t* countLabel = nullptr;
    lv_obj_t* aircraftLabel = nullptr;
};
