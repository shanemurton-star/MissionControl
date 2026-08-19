#pragma once
#include <functional>
#include <lvgl.h>
#include "../services/LiveSpotsService.h"

class LiveSpotsPanel
{
public:
    using ClickCallback = std::function<void()>;
    void create(lv_obj_t* parent, LiveSpotsService& service, int16_t x, int16_t y, int16_t width, int16_t height);
    void update();
    void setClickCallback(ClickCallback callback);
private:
    static void panelEventHandler(lv_event_t* event);
    LiveSpotsService* service = nullptr;
    ClickCallback clickCallback;
    lv_obj_t* panel = nullptr;
    lv_obj_t* totalLabel = nullptr;
    lv_obj_t* bandsLabel = nullptr;
    lv_obj_t* bandCells[9] = {};
};
