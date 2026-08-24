#pragma once
#include <Arduino.h>
#include <lvgl.h>
namespace DashboardIcons
{
    enum class Type { Weather, Aircraft, Radio, Solar, Satellite, Pota };
    lv_obj_t* create(lv_obj_t*, Type, int16_t, int16_t, uint32_t);
    void setWeatherCondition(lv_obj_t* image, const String& condition);
    void warmCache(const String& weatherCondition);
}
