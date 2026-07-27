#pragma once

#include <lvgl.h>

class StartupScreen
{
public:
    void begin();
    void show();

private:
    lv_obj_t* screen = nullptr;
};