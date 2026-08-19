#include "Theme.h"

namespace Theme
{
    lv_color_t color(uint32_t value)
    {
        return lv_color_hex(value);
    }

    void configureScreen(lv_obj_t* object)
    {
        if (object == nullptr)
        {
            return;
        }

        lv_obj_set_style_bg_color(
            object,
            color(COLOR_BACKGROUND),
            LV_PART_MAIN);

        lv_obj_set_style_bg_opa(
            object,
            LV_OPA_COVER,
            LV_PART_MAIN);

        lv_obj_set_style_border_width(
            object,
            0,
            LV_PART_MAIN);

        lv_obj_set_style_pad_all(
            object,
            0,
            LV_PART_MAIN);

        lv_obj_clear_flag(
            object,
            LV_OBJ_FLAG_SCROLLABLE);
    }

    void configureHeader(lv_obj_t* object)
    {
        if (object == nullptr)
        {
            return;
        }

        lv_obj_set_style_bg_color(
            object,
            color(COLOR_HEADER),
            LV_PART_MAIN);

        lv_obj_set_style_bg_opa(
            object,
            LV_OPA_COVER,
            LV_PART_MAIN);

        lv_obj_set_style_border_width(
            object,
            0,
            LV_PART_MAIN);

        lv_obj_set_style_radius(
            object,
            0,
            LV_PART_MAIN);

        lv_obj_set_style_pad_all(
            object,
            0,
            LV_PART_MAIN);

        lv_obj_clear_flag(
            object,
            LV_OBJ_FLAG_SCROLLABLE);
    }

    void configurePanel(lv_obj_t* object)
    {
        if (object == nullptr)
        {
            return;
        }

        lv_obj_set_style_bg_color(
            object,
            color(COLOR_PANEL),
            LV_PART_MAIN);

        lv_obj_set_style_bg_opa(
            object,
            LV_OPA_COVER,
            LV_PART_MAIN);

        lv_obj_set_style_border_color(
            object,
            color(COLOR_PANEL_BORDER),
            LV_PART_MAIN);

        lv_obj_set_style_border_width(
            object,
            1,
            LV_PART_MAIN);

        lv_obj_set_style_radius(
            object,
            8,
            LV_PART_MAIN);

        lv_obj_set_style_pad_all(
            object,
            10,
            LV_PART_MAIN);

        lv_obj_clear_flag(
            object,
            LV_OBJ_FLAG_SCROLLABLE);
    }

    lv_obj_t* createLabel(
        lv_obj_t* parent,
        const char* text,
        uint32_t textColor,
        const lv_font_t* font)
    {
        lv_obj_t* label = lv_label_create(parent);

        lv_label_set_text(
            label,
            text);

        lv_obj_set_style_text_color(
            label,
            color(textColor),
            LV_PART_MAIN);

        lv_obj_set_style_text_font(
            label,
            font,
            LV_PART_MAIN);

        return label;
    }

    lv_obj_t* createPanel(
        lv_obj_t* parent,
        int16_t x,
        int16_t y,
        int16_t width,
        int16_t height,
        const char* title)
    {
        lv_obj_t* panel = lv_obj_create(parent);

        lv_obj_set_pos(
            panel,
            x,
            y);

        lv_obj_set_size(
            panel,
            width,
            height);

        configurePanel(panel);

        lv_obj_t* titleLabel = createLabel(
            panel,
            title,
            COLOR_PRIMARY);

        lv_obj_align(
            titleLabel,
            LV_ALIGN_TOP_LEFT,
            0,
            0);

        return panel;
    }

    void createDivider(
        lv_obj_t* parent,
        int16_t x,
        int16_t y,
        int16_t width)
    {
        lv_obj_t* divider = lv_obj_create(parent);

        lv_obj_set_pos(
            divider,
            x,
            y);

        lv_obj_set_size(
            divider,
            width,
            1);

        lv_obj_set_style_bg_color(
            divider,
            color(COLOR_PANEL_BORDER),
            LV_PART_MAIN);

        lv_obj_set_style_bg_opa(
            divider,
            LV_OPA_COVER,
            LV_PART_MAIN);

        lv_obj_set_style_border_width(
            divider,
            0,
            LV_PART_MAIN);

        lv_obj_set_style_pad_all(
            divider,
            0,
            LV_PART_MAIN);

        lv_obj_clear_flag(
            divider,
            LV_OBJ_FLAG_SCROLLABLE);
    }
}
