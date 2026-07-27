#pragma once

#include <functional>

#include <lvgl.h>

#include "../models/Page.h"

class NavigationBar
{
public:

    using NavigationCallback =
        std::function<void(Page)>;

    void create(
        lv_obj_t* parent);

    void setSelected(
        Page page);

    void setCallback(
        NavigationCallback callback);

private:

    static void buttonEventHandler(
        lv_event_t* event);

    void styleButton(
        lv_obj_t* button,
        bool selected);

    NavigationCallback callback;

    lv_obj_t* container = nullptr;

    lv_obj_t* dashboardButton = nullptr;
    lv_obj_t* weatherButton = nullptr;
    lv_obj_t* hamButton = nullptr;
    lv_obj_t* systemButton = nullptr;
    lv_obj_t* settingsButton = nullptr;
};