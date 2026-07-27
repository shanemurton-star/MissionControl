#include "StartupScreen.h"

void StartupScreen::begin()
{
    // Do not create the screen more than once.
    if (screen != nullptr)
    {
        return;
    }

    screen = lv_obj_create(nullptr);

    lv_obj_set_style_bg_color(
        screen,
        lv_color_hex(0x07111F),
        LV_PART_MAIN);

    lv_obj_set_style_bg_opa(
        screen,
        LV_OPA_COVER,
        LV_PART_MAIN);

    lv_obj_clear_flag(
        screen,
        LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(screen);

    lv_label_set_text(
        title,
        "MISSION CONTROL");

    lv_obj_set_style_text_color(
        title,
        lv_color_hex(0xFFFFFF),
        LV_PART_MAIN);

    lv_obj_set_style_text_font(
        title,
        &lv_font_montserrat_14,
        LV_PART_MAIN);

    lv_obj_align(
        title,
        LV_ALIGN_CENTER,
        0,
        -70);

    lv_obj_t* subtitle = lv_label_create(screen);

    lv_label_set_text(
        subtitle,
        "Initializing...");

    lv_obj_set_style_text_color(
        subtitle,
        lv_color_hex(0x8EA9C1),
        LV_PART_MAIN);

    lv_obj_set_style_text_font(
        subtitle,
        &lv_font_montserrat_14,
        LV_PART_MAIN);

    lv_obj_align(
        subtitle,
        LV_ALIGN_CENTER,
        0,
        0);

    lv_obj_t* status = lv_label_create(screen);

    lv_label_set_text(
        status,
        "Display ready");

    lv_obj_set_style_text_color(
        status,
        lv_color_hex(0x57D68D),
        LV_PART_MAIN);

    lv_obj_set_style_text_font(
        status,
        &lv_font_montserrat_14,
        LV_PART_MAIN);

    lv_obj_align(
        status,
        LV_ALIGN_CENTER,
        0,
        55);

    lv_obj_t* version = lv_label_create(screen);

    lv_label_set_text(
        version,
        "Firmware v0.1");

    lv_obj_set_style_text_color(
        version,
        lv_color_hex(0x60758A),
        LV_PART_MAIN);

    lv_obj_set_style_text_font(
        version,
        &lv_font_montserrat_14,
        LV_PART_MAIN);

    lv_obj_align(
        version,
        LV_ALIGN_BOTTOM_MID,
        0,
        -25);
}

void StartupScreen::show()
{
    if (screen == nullptr)
    {
        begin();
    }

    lv_scr_load(screen);
}