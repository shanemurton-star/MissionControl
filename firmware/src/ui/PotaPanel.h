#pragma once

#include <functional>
#include <lvgl.h>

#include "../services/PotaService.h"

class PotaPanel
{
public:
    using ClickCallback = std::function<void()>;

    void create(
        lv_obj_t* parent,
        PotaService& service,
        int16_t x,
        int16_t y,
        int16_t width,
        int16_t height);
    void update();
    void setClickCallback(ClickCallback callback);

private:
    static constexpr uint8_t PANEL_SPOTS = 3;
    static void panelEventHandler(lv_event_t* event);
    PotaService* service = nullptr;
    ClickCallback clickCallback;
    lv_obj_t* panel = nullptr;
    lv_obj_t* statusLabel = nullptr;
    lv_obj_t* countLabel = nullptr;
    lv_obj_t* spotLabels[PANEL_SPOTS] = {};
};
