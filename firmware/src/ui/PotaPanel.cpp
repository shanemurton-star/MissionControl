#include "PotaPanel.h"

#include <WiFi.h>

#include "Theme.h"
#include "DashboardIcons.h"

void PotaPanel::create(
    lv_obj_t* parent,
    PotaService& serviceReference,
    int16_t x,
    int16_t y,
    int16_t width,
    int16_t height)
{
    service = &serviceReference;
    panel = Theme::createPanel(parent, x, y, width, height, "POTA SPOTS");
    lv_obj_add_flag(panel, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(panel, panelEventHandler, LV_EVENT_CLICKED, this);
    lv_obj_t* title=lv_obj_get_child(panel,0); lv_obj_set_style_text_color(title,Theme::color(Theme::COLOR_TEXT),0); lv_obj_align(title,LV_ALIGN_TOP_MID,0,0);
    DashboardIcons::create(panel,DashboardIcons::Type::Pota,12,43,0xB02BFA);
    countLabel=Theme::createLabel(panel,"--",Theme::COLOR_TEXT,&lv_font_montserrat_28); lv_obj_align(countLabel,LV_ALIGN_TOP_MID,40,50);
    statusLabel = Theme::createLabel(panel, "WAITING FOR POTA", Theme::COLOR_TEXT_MUTED);
    lv_obj_set_width(statusLabel,width-24); lv_obj_set_style_text_align(statusLabel,LV_TEXT_ALIGN_CENTER,0); lv_obj_align(statusLabel,LV_ALIGN_BOTTOM_MID,0,-4);

    for (uint8_t index = 0; index < PANEL_SPOTS; ++index)
    {
        spotLabels[index] = Theme::createLabel(panel, "", Theme::COLOR_TEXT);
        lv_obj_set_pos(spotLabels[index], 0, 28 + index * 40);
        lv_obj_set_width(spotLabels[index], width - 22);
        lv_obj_add_flag(spotLabels[index], LV_OBJ_FLAG_HIDDEN);
    }
    update();
}

void PotaPanel::update()
{
    if (service == nullptr) return;
    if (!service->isValid())
    {
        lv_label_set_text(countLabel, "--");
        lv_obj_clear_flag(statusLabel, LV_OBJ_FLAG_HIDDEN);
        if (WiFi.status() != WL_CONNECTED) lv_label_set_text(statusLabel, "WAITING FOR WIFI");
        else if (service->isUpdating()) lv_label_set_text(statusLabel, "UPDATING POTA SPOTS...");
        else if (!service->getLastError().isEmpty()) lv_label_set_text(statusLabel, service->getLastError().c_str());
        return;
    }

    lv_obj_add_flag(statusLabel, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(countLabel, String(service->getSpotCount()).c_str());
    for (uint8_t index = 0; index < PANEL_SPOTS; ++index)
    {
        if (index >= service->getSpotCount())
        {
            lv_obj_add_flag(spotLabels[index], LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        lv_obj_add_flag(spotLabels[index], LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_clear_flag(statusLabel,LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(statusLabel,"Active Parks\nWithin 100 mi");
}

void PotaPanel::setClickCallback(ClickCallback callback) { clickCallback = callback; }
void PotaPanel::panelEventHandler(lv_event_t* event)
{
    PotaPanel* self = static_cast<PotaPanel*>(lv_event_get_user_data(event));
    if (self != nullptr && self->clickCallback != nullptr) self->clickCallback();
}
