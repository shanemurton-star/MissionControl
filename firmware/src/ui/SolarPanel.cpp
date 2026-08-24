#include "SolarPanel.h"
#include <WiFi.h>
#include "Theme.h"
#include "DashboardIcons.h"

void SolarPanel::create(lv_obj_t* parent, SolarService& serviceReference,
                        int16_t x, int16_t y, int16_t width, int16_t height)
{
    service = &serviceReference;
    panel = Theme::createPanel(parent, x, y, width, height, "SOLAR");
    lv_obj_add_flag(panel, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(panel, panelEventHandler, LV_EVENT_CLICKED, this);
    lv_obj_t* title=lv_obj_get_child(panel,0); lv_obj_set_style_text_color(title,Theme::color(Theme::COLOR_TEXT),0); lv_obj_align(title,LV_ALIGN_TOP_MID,0,0);
    // Combine scaled versions of the existing solar and LIVE SPOTS assets.
    // Reusing the exact tower silhouette keeps the dashboard icon language
    // consistent and avoids the antenna looking like a letter A.
    lv_obj_t* sun = DashboardIcons::create(
        panel, DashboardIcons::Type::Solar, 0, 36, 0xF4A900);
    lv_img_set_zoom(sun, 160);
    lv_obj_t* tower = DashboardIcons::create(
        panel, DashboardIcons::Type::Radio, 16, 38, 0xF4A900);
    lv_img_set_zoom(tower, 160);
    lv_obj_set_style_img_recolor(
        tower, Theme::color(0xAEB8C4), LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(
        tower, LV_OPA_COVER, LV_PART_MAIN);

    detailsLabel = Theme::createLabel(panel, "WAITING FOR SPACE WEATHER", Theme::COLOR_TEXT_MUTED);
    lv_obj_set_width(detailsLabel,width-92); lv_obj_set_style_text_align(detailsLabel,LV_TEXT_ALIGN_CENTER,0); lv_obj_align(detailsLabel,LV_ALIGN_RIGHT_MID,-2,15);
    update();
}

void SolarPanel::update()
{
    if (service == nullptr || detailsLabel == nullptr) return;
    if (!service->isValid())
    {
        if (WiFi.status() != WL_CONNECTED) lv_label_set_text(detailsLabel, "WAITING FOR WIFI");
        else if (service->isUpdating()) lv_label_set_text(detailsLabel, "UPDATING NOAA DATA...");
        else if (!service->getLastError().isEmpty()) lv_label_set_text(detailsLabel, service->getLastError().c_str());
        return;
    }
    const SolarData& data = service->getData();
    String text = "SFI  " + String(data.solarFlux, 0) +
        "\nKp  " + String(data.kpIndex, 1) +
        "\n" + service->getPropagationLabel();
    lv_label_set_text(detailsLabel, text.c_str());
}

void SolarPanel::setClickCallback(ClickCallback callback) { clickCallback = callback; }
void SolarPanel::panelEventHandler(lv_event_t* event)
{
    SolarPanel* self = static_cast<SolarPanel*>(lv_event_get_user_data(event));
    if (self != nullptr && self->clickCallback != nullptr) self->clickCallback();
}
