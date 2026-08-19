#include "AircraftPanel.h"

#include <WiFi.h>
#include "Theme.h"
#include "DashboardIcons.h"

void AircraftPanel::create(lv_obj_t* parent, AircraftService& serviceReference,
                           int16_t x, int16_t y, int16_t width, int16_t height)
{
    service = &serviceReference;
    panel = Theme::createPanel(parent, x, y, width, height, "AIRCRAFT");
    lv_obj_add_flag(panel, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(panel, panelEventHandler, LV_EVENT_CLICKED, this);

    lv_obj_t* title = lv_obj_get_child(panel, 0);
    lv_obj_set_style_text_color(title, Theme::color(Theme::COLOR_TEXT), LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);
    DashboardIcons::create(panel, DashboardIcons::Type::Aircraft, 12, 43, 0x52E018);
    countLabel = Theme::createLabel(panel, "--", Theme::COLOR_TEXT, &lv_font_montserrat_28);
    lv_obj_align(countLabel, LV_ALIGN_TOP_MID, 38, 50);

    aircraftLabel = Theme::createLabel(panel, "WAITING FOR LIVE ADS-B", Theme::COLOR_TEXT_MUTED);
    lv_obj_set_style_text_align(aircraftLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_width(aircraftLabel, width - 24);
    lv_obj_align(aircraftLabel, LV_ALIGN_BOTTOM_MID, 0, -4);
    update();
}

void AircraftPanel::update()
{
    if (service == nullptr || countLabel == nullptr || aircraftLabel == nullptr) return;

    if (!service->isValid())
    {
        lv_label_set_text(countLabel, "--");
        if (WiFi.status() != WL_CONNECTED)
            lv_label_set_text(aircraftLabel, "WAITING FOR WIFI");
        else if (service->isUpdating())
            lv_label_set_text(aircraftLabel, "UPDATING LIVE TRAFFIC...");
        else if (!service->getLastError().isEmpty())
            lv_label_set_text(aircraftLabel, service->getLastError().c_str());
        else
            lv_label_set_text(aircraftLabel, "WAITING FOR LIVE ADS-B");
        return;
    }

    const uint8_t count = service->getAircraftCount();
    String countText = String(count);
    lv_label_set_text(countLabel, countText.c_str());

    lv_label_set_text(aircraftLabel, "In Range\nADS-B Active");
}

void AircraftPanel::setClickCallback(ClickCallback callback) { clickCallback = callback; }

void AircraftPanel::panelEventHandler(lv_event_t* event)
{
    AircraftPanel* self = static_cast<AircraftPanel*>(lv_event_get_user_data(event));
    if (self != nullptr && self->clickCallback != nullptr) self->clickCallback();
}
