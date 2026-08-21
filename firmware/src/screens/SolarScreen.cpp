#include "SolarScreen.h"
#include "../services/NetworkUpdateState.h"
#include <WiFi.h>
#include <time.h>
#include "../ui/Theme.h"

namespace
{
    const char* ratingLabel(int rating)
    {
        if (rating >= 2) return "GOOD";
        if (rating == 1) return "FAIR";
        return "POOR";
    }

    int disturbancePenalty(const SolarData& data)
    {
        if (data.kpIndex >= 6.0f || data.aIndex >= 40) return 2;
        if (data.kpIndex >= 4.0f || data.aIndex >= 20) return 1;
        return 0;
    }

    int bandRating(uint8_t bandIndex, bool daytime, const SolarData& data)
    {
        int rating = 0;
        switch (bandIndex)
        {
            case 0: rating = daytime ? 0 : 2; break; // 160m
            case 1: rating = daytime ? 1 : 2; break; // 80m
            case 2: rating = daytime ? 1 : 2; break; // 60m
            case 3: rating = 2; break;               // 40m
            case 4: rating = 2; break;               // 30m
            case 5: rating = 2; break;               // 20m
            case 6: rating = daytime ? (data.solarFlux >= 90.0f ? 2 : 1) : 1; break;
            case 7:
                rating = daytime ? (data.solarFlux >= 110.0f ? 2 :
                                    (data.solarFlux >= 85.0f ? 1 : 0)) :
                                   (data.solarFlux >= 150.0f ? 1 : 0);
                break;
            case 8:
                rating = daytime ? (data.solarFlux >= 130.0f ? 2 :
                                    (data.solarFlux >= 100.0f ? 1 : 0)) : 0;
                break;
            case 9:
                rating = daytime ? (data.solarFlux >= 150.0f ? 2 :
                                    (data.solarFlux >= 110.0f ? 1 : 0)) : 0;
                break;
            case 10:
                rating = daytime ? (data.solarFlux >= 180.0f ? 2 :
                                    (data.solarFlux >= 140.0f ? 1 : 0)) : 0;
                break;
        }

        rating -= disturbancePenalty(data);
        if (daytime && data.radioBlackoutScale > 0)
            rating -= data.radioBlackoutScale >= 3 ? 2 : 1;
        if (rating < 0) rating = 0;
        return rating;
    }
}

void SolarScreen::begin(ClockService& clockService, SolarService& service)
{
    if (screen != nullptr) return;
    solarService = &service;
    screen = lv_obj_create(nullptr);
    Theme::configureScreen(screen);
    headerBar.create(screen, clockService, Theme::SCREEN_WIDTH, Theme::HEADER_HEIGHT);
    headerBar.setSettingsCallback([this]() {
        if (navigationCallback != nullptr) navigationCallback(Page::Settings);
    });
    headerBar.setNavigationCallback([this](Page page) {
        if (navigationCallback != nullptr) navigationCallback(page);
    });
    lv_obj_t* backButton = lv_btn_create(screen);
    lv_obj_set_pos(backButton, 8, Theme::CONTENT_TOP);
    lv_obj_set_size(backButton, 116, 36);
    lv_obj_set_style_bg_color(backButton, Theme::color(Theme::COLOR_PANEL), LV_PART_MAIN);
    lv_obj_set_style_border_color(backButton, Theme::color(Theme::COLOR_PANEL_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(backButton, 1, LV_PART_MAIN);
    lv_obj_add_event_cb(backButton, backButtonEventHandler, LV_EVENT_CLICKED, this);
    lv_obj_center(Theme::createLabel(backButton, LV_SYMBOL_LEFT " DASHBOARD", Theme::COLOR_PRIMARY));
    lv_obj_t* title = Theme::createLabel(screen, "SOLAR CONDITIONS DETAIL", Theme::COLOR_PRIMARY);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 140, Theme::CONTENT_TOP + 10);

    lv_obj_t* overview = Theme::createPanel(screen, 8, 110, 252, 328, "HF PROPAGATION");
    conditionLabel = Theme::createLabel(overview, "UNKNOWN", Theme::COLOR_SUCCESS, &lv_font_montserrat_14);
    lv_obj_align(conditionLabel, LV_ALIGN_TOP_MID, 0, 32);
    explanationLabel = Theme::createLabel(overview, "WAITING FOR NOAA SWPC", Theme::COLOR_TEXT_MUTED);
    lv_obj_set_width(explanationLabel, 220);
    lv_label_set_long_mode(explanationLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(explanationLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(explanationLabel, LV_ALIGN_TOP_MID, 0, 58);

    bandOutlookLabels[0] = Theme::createLabel(overview, "BAND", Theme::COLOR_PRIMARY);
    bandOutlookLabels[1] = Theme::createLabel(overview, "DAY", Theme::COLOR_PRIMARY);
    bandOutlookLabels[2] = Theme::createLabel(overview, "NIGHT", Theme::COLOR_PRIMARY);
    const int16_t columnX[] = {0, 60, 145};
    const int16_t columnWidth[] = {55, 80, 80};
    for (uint8_t column = 0; column < 3; ++column)
    {
        lv_obj_set_pos(bandOutlookLabels[column], columnX[column], 108);
        lv_obj_set_width(bandOutlookLabels[column], columnWidth[column]);
        lv_obj_set_style_text_align(bandOutlookLabels[column], LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    }

    lv_obj_t* metrics = Theme::createPanel(screen, 268, 110, 252, 328, "LIVE SOLAR METRICS");
    metricsLabel = Theme::createLabel(metrics, "WAITING FOR DATA", Theme::COLOR_TEXT_MUTED);
    lv_obj_align(metricsLabel, LV_ALIGN_TOP_LEFT, 0, 36);

    lv_obj_t* scales = Theme::createPanel(screen, 528, 110, 264, 328, "NOAA SPACE WEATHER SCALES");
    scalesLabel = Theme::createLabel(scales, "R--  S--  G--", Theme::COLOR_TEXT_MUTED);
    lv_obj_align(scalesLabel, LV_ALIGN_TOP_LEFT, 0, 36);
    update();
    updateTimer = lv_timer_create(updateTimerCallback, 1000, this);
}

void SolarScreen::show() { if (screen != nullptr) { lv_scr_load(screen); update(); } }
void SolarScreen::setNavigationCallback(NavigationCallback callback) { navigationCallback = callback; }

void SolarScreen::update()
{
    headerBar.update();
    if (NetworkUpdateState::isBusy()) return;
    if (solarService == nullptr) return;
    if (!solarService->isValid())
    {
        const char* status = WiFi.status() != WL_CONNECTED ? "WAITING FOR WIFI" :
            (solarService->isUpdating() ? "UPDATING NOAA SWPC..." : solarService->getLastError().c_str());
        lv_label_set_text(explanationLabel, status);
        return;
    }
    const SolarData& data = solarService->getData();
    lv_label_set_text(conditionLabel, solarService->getPropagationLabel());
    const bool disturbed = data.kpIndex >= 4.0f || data.radioBlackoutScale > 0;
    lv_obj_set_style_text_color(conditionLabel,
        Theme::color(disturbed ? Theme::COLOR_WARNING : Theme::COLOR_SUCCESS), LV_PART_MAIN);
    lv_label_set_text(explanationLabel,
        disturbed ? "Disturbed conditions may reduce HF paths."
                  : "Estimated outlook from current solar data.");

    static const char* bandNames[] = {
        "160m", "80m", "60m", "40m", "30m", "20m",
        "17m", "15m", "12m", "10m", "6m"
    };
    String bands = "BAND";
    String day = "DAY";
    String night = "NIGHT";
    for (uint8_t index = 0; index < 11; ++index)
    {
        bands += "\n" + String(bandNames[index]);
        day += "\n" + String(ratingLabel(bandRating(index, true, data)));
        night += "\n" + String(ratingLabel(bandRating(index, false, data)));
    }
    lv_label_set_text(bandOutlookLabels[0], bands.c_str());
    lv_label_set_text(bandOutlookLabels[1], day.c_str());
    lv_label_set_text(bandOutlookLabels[2], night.c_str());

    String metrics = "F10.7 Solar Flux\n  " + String(data.solarFlux, 0) + " sfu\n\n" +
        "Planetary Kp\n  " + String(data.kpIndex, 1) + "\n\n" +
        "Running A Index\n  " + String(data.aIndex) + "\n\n" +
        "GOES X-Ray\n  " + data.xrayClass + "  (peak " + data.flarePeakClass + ")\n\n" +
        "Solar Wind\n  " + String(data.solarWindSpeed, 0) + " km/s\n\n" +
        "Magnetic Field Bt\n  " + String(data.magneticField, 1) + " nT";
    lv_label_set_text(metricsLabel, metrics.c_str());

    String scales = "R" + String(data.radioBlackoutScale) + "  RADIO BLACKOUT\n" +
        (data.radioBlackoutScale == 0 ? "None" : "Active") + "\n\n" +
        "S" + String(data.solarRadiationScale) + "  RADIATION STORM\n" +
        (data.solarRadiationScale == 0 ? "None" : "Active") + "\n\n" +
        "G" + String(data.geomagneticStormScale) + "  GEOMAGNETIC\n" +
        (data.geomagneticStormScale == 0 ? "None" : "Active") +
        "\n\nUpdated from NOAA SWPC\nevery 5 minutes";
    lv_label_set_text(scalesLabel, scales.c_str());
}

void SolarScreen::backButtonEventHandler(lv_event_t* event)
{
    SolarScreen* self = static_cast<SolarScreen*>(lv_event_get_user_data(event));
    if (self != nullptr && self->navigationCallback != nullptr) self->navigationCallback(Page::Dashboard);
}
void SolarScreen::updateTimerCallback(lv_timer_t* timer)
{
    SolarScreen* self = static_cast<SolarScreen*>(timer->user_data);
    if (self != nullptr) self->update();
}
