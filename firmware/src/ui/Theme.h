#pragma once

#include <lvgl.h>

namespace Theme
{
    // Screen dimensions
    constexpr int16_t SCREEN_WIDTH = 800;
    constexpr int16_t SCREEN_HEIGHT = 480;

    // Shared layout
    constexpr int16_t HEADER_HEIGHT = 64;
    constexpr int16_t FOOTER_HEIGHT = 34;
    constexpr int16_t FOOTER_TOP = SCREEN_HEIGHT - FOOTER_HEIGHT;
    constexpr int16_t CONTENT_TOP = HEADER_HEIGHT + 8;
    constexpr int16_t CONTENT_BOTTOM = FOOTER_TOP - 8;

    // Mission Control color palette
    constexpr uint32_t COLOR_BACKGROUND = 0x11191D;
    constexpr uint32_t COLOR_HEADER = 0x030B10;
    constexpr uint32_t COLOR_PANEL = 0x02080C;
    constexpr uint32_t COLOR_PANEL_BORDER = 0x1B272D;

    constexpr uint32_t COLOR_PRIMARY = 0x32C7E8;
    constexpr uint32_t COLOR_TEXT = 0xFFFFFF;
    constexpr uint32_t COLOR_TEXT_MUTED = 0x8EA9C1;
    constexpr uint32_t COLOR_TEXT_DIM = 0x7893A8;

    constexpr uint32_t COLOR_SUCCESS = 0x57D68D;
    constexpr uint32_t COLOR_WARNING = 0xF4C95D;
    constexpr uint32_t COLOR_ERROR = 0xF06A6A;

    lv_color_t color(uint32_t value);

    void configureScreen(lv_obj_t* object);
    void configureHeader(lv_obj_t* object);
    void configurePanel(lv_obj_t* object);
    lv_obj_t* createLabel(
        lv_obj_t* parent,
        const char* text,
        uint32_t textColor = COLOR_TEXT,
        const lv_font_t* font = &lv_font_montserrat_14);

    lv_obj_t* createPanel(
        lv_obj_t* parent,
        int16_t x,
        int16_t y,
        int16_t width,
        int16_t height,
        const char* title);

    void createDivider(
        lv_obj_t* parent,
        int16_t x,
        int16_t y,
        int16_t width);
}
