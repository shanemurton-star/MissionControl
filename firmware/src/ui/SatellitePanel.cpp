#include "SatellitePanel.h"

#include <WiFi.h>
#include <time.h>
#include "Theme.h"
#include "DashboardIcons.h"

namespace
{
    String formatTime(uint32_t timestamp)
    {
        time_t value = timestamp;
        struct tm timeInfo;
        if (localtime_r(&value, &timeInfo) == nullptr) return "--:--";
        char buffer[8];
        strftime(buffer, sizeof(buffer), "%H:%M", &timeInfo);
        return buffer;
    }

    String compactSatelliteName(const String& name)
    {
        if (name.startsWith("ISS")) return "ISS";
        return name;
    }

    const char* direction(float azimuth)
    {
        static const char* directions[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
        return directions[static_cast<uint8_t>((azimuth + 22.5f) / 45.0f) % 8];
    }
}

void SatellitePanel::create(lv_obj_t* parent, SatelliteService& serviceReference,
                            int16_t x, int16_t y, int16_t width, int16_t height)
{
    service = &serviceReference;
    panel = Theme::createPanel(parent, x, y, width, height, "SATELLITES");
    lv_obj_add_flag(panel, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(panel, panelEventHandler, LV_EVENT_CLICKED, this);
    lv_obj_t* title=lv_obj_get_child(panel,0); lv_obj_set_style_text_color(title,Theme::color(Theme::COLOR_TEXT),0); lv_obj_align(title,LV_ALIGN_TOP_MID,0,0);
    DashboardIcons::create(panel,DashboardIcons::Type::Satellite,12,43,0x1598F2);
    nameLabel = Theme::createLabel(panel, "WAITING FOR ORBIT DATA", Theme::COLOR_TEXT_MUTED);
    lv_obj_set_width(nameLabel,width-24); lv_obj_set_style_text_align(nameLabel,LV_TEXT_ALIGN_CENTER,0); lv_obj_align(nameLabel,LV_ALIGN_BOTTOM_MID,0,-4);
    countdownLabel = Theme::createLabel(panel, "--", Theme::COLOR_TEXT, &lv_font_montserrat_28);
    lv_obj_align(countdownLabel, LV_ALIGN_TOP_MID, 40, 50);
    detailsLabel = Theme::createLabel(panel,
        "AOS          --:--\nMax Elev     -- deg\nDirection    ---\nDuration     -- min",
        Theme::COLOR_TEXT_MUTED);
    lv_obj_add_flag(detailsLabel, LV_OBJ_FLAG_HIDDEN);
    update();
}

void SatellitePanel::update()
{
    if (service == nullptr) return;

    uint8_t visible = 0;
    int8_t highestSatellite = -1;
    int8_t secondHighestSatellite = -1;
    float highestElevation = -91.0f;
    float secondHighestElevation = -91.0f;

    if (service->isValid())
    {
        for (uint8_t i = 0; i < SatelliteService::SATELLITE_COUNT; ++i)
        {
            const SatelliteData& satellite = service->getSatellite(i);
            if (!satellite.valid || !satellite.visible) continue;

            ++visible;
            if (satellite.currentElevation > highestElevation)
            {
                secondHighestElevation = highestElevation;
                secondHighestSatellite = highestSatellite;
                highestElevation = satellite.currentElevation;
                highestSatellite = static_cast<int8_t>(i);
            }
            else if (satellite.currentElevation > secondHighestElevation)
            {
                secondHighestElevation = satellite.currentElevation;
                secondHighestSatellite = static_cast<int8_t>(i);
            }
        }
    }

    if (visible > 0 && highestSatellite >= 0)
    {
        String activeText = compactSatelliteName(service->getSatellite(highestSatellite).name);
        if (visible == 2 && secondHighestSatellite >= 0)
        {
            activeText += " + " + compactSatelliteName(service->getSatellite(secondHighestSatellite).name);
        }
        else if (visible > 2)
        {
            activeText += " +" + String(visible - 1) + " MORE";
        }
        activeText += visible == 1 ? "\nIN PASS" : "\n" + String(visible) + " IN PASS";

        lv_label_set_text(nameLabel, activeText.c_str());
        lv_label_set_text(countdownLabel, String(visible).c_str());
        return;
    }

    const SatelliteData* pass = service->getNextPass();
    if (!service->isValid() || pass == nullptr)
    {
        if (WiFi.status() != WL_CONNECTED) lv_label_set_text(nameLabel, "WAITING FOR WIFI");
        else if (service->isUpdating()) lv_label_set_text(nameLabel, "UPDATING ORBITS...");
        else if (!service->getLastError().isEmpty()) lv_label_set_text(nameLabel, service->getLastError().c_str());
        lv_label_set_text(countdownLabel, "--");
        return;
    }

    String passText = pass->name + "\nNext " + formatTime(pass->aosTime);
    lv_label_set_text(nameLabel, passText.c_str());
    lv_label_set_text(countdownLabel, String(visible).c_str());
    const uint32_t durationMinutes = (pass->losTime - pass->aosTime + 30) / 60;
    String details = "AOS          " + formatTime(pass->aosTime) +
        "\nMax Elev     " + String(pass->maxElevation, 0) + " deg" +
        "\nDirection    " + direction(pass->aosAzimuth) + " to " + direction(pass->losAzimuth) +
        "\nDuration     " + String(durationMinutes) + " min";
    lv_label_set_text(detailsLabel, details.c_str());
}

void SatellitePanel::setClickCallback(ClickCallback callback) { clickCallback = callback; }
void SatellitePanel::panelEventHandler(lv_event_t* event)
{
    SatellitePanel* self = static_cast<SatellitePanel*>(lv_event_get_user_data(event));
    if (self != nullptr && self->clickCallback != nullptr) self->clickCallback();
}
