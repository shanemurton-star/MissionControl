#include "LiveSpotsPanel.h"
#include <WiFi.h>
#include "Theme.h"
#include "DashboardIcons.h"

void LiveSpotsPanel::create(lv_obj_t* parent, LiveSpotsService& serviceReference,
                            int16_t x, int16_t y, int16_t width, int16_t height)
{
    service = &serviceReference;
    panel = Theme::createPanel(parent, x, y, width, height, "LIVE SPOTS");
    lv_obj_add_flag(panel, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(panel, panelEventHandler, LV_EVENT_CLICKED, this);
    lv_obj_t* title = lv_obj_get_child(panel, 0); lv_obj_set_style_text_color(title, Theme::color(Theme::COLOR_TEXT), 0); lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);
    DashboardIcons::create(panel, DashboardIcons::Type::Radio, 12, 43, 0xF4A900);
    totalLabel = Theme::createLabel(panel, "--", Theme::COLOR_TEXT, &lv_font_montserrat_28);
    lv_obj_align(totalLabel, LV_ALIGN_TOP_MID, 38, 50);
    bandsLabel = Theme::createLabel(panel, "WAITING FOR PSK REPORTER", Theme::COLOR_TEXT_MUTED);
    lv_obj_set_width(bandsLabel, width - 24); lv_obj_set_style_text_align(bandsLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(bandsLabel, LV_ALIGN_BOTTOM_MID, 0, -4);

    constexpr int16_t cellWidth = 77;
    constexpr int16_t cellHeight = 30;
    for (uint8_t index = 0; index < 9; ++index)
    {
        bandCells[index] = Theme::createLabel(panel, "--\n--", Theme::COLOR_TEXT);
        lv_obj_set_pos(
            bandCells[index],
            (index % 3) * cellWidth,
            56 + (index / 3) * cellHeight);
        lv_obj_set_size(bandCells[index], cellWidth, cellHeight);
        lv_obj_set_style_text_align(bandCells[index], LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_add_flag(bandCells[index], LV_OBJ_FLAG_HIDDEN);
    }
    update();
}

void LiveSpotsPanel::update()
{
    if (service == nullptr) return;
    if (!service->isValid())
    {
        lv_obj_clear_flag(bandsLabel, LV_OBJ_FLAG_HIDDEN);
        for (lv_obj_t* cell : bandCells) lv_obj_add_flag(cell, LV_OBJ_FLAG_HIDDEN);
        if (WiFi.status() != WL_CONNECTED) lv_label_set_text(bandsLabel, "WAITING FOR WIFI");
        else if (service->isUpdating()) lv_label_set_text(bandsLabel, "UPDATING LIVE SPOTS...");
        else if (!service->getLastError().isEmpty()) lv_label_set_text(bandsLabel, service->getLastError().c_str());
        return;
    }
    String total = String(service->getTotalSpotCount());
    lv_label_set_text(totalLabel, total.c_str());
    lv_obj_clear_flag(bandsLabel, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(bandsLabel, "Spots\nLast Hour");
    for (lv_obj_t* cell : bandCells) lv_obj_add_flag(cell, LV_OBJ_FLAG_HIDDEN);
}

void LiveSpotsPanel::setClickCallback(ClickCallback callback) { clickCallback = callback; }
void LiveSpotsPanel::panelEventHandler(lv_event_t* event)
{
    LiveSpotsPanel* self = static_cast<LiveSpotsPanel*>(lv_event_get_user_data(event));
    if (self != nullptr && self->clickCallback != nullptr) self->clickCallback();
}
