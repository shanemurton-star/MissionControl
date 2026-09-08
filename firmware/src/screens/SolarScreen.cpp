#include "SolarScreen.h"
#include "../services/NetworkUpdateState.h"
#include <WiFi.h>
#include <time.h>
#include "../ui/Theme.h"

namespace
{
    constexpr uint32_t SOLAR_BAND_COLORS[] = {
        0xE85B3A, // 160m - red-orange
        0xFF8C32, // 80m - orange
        0xE8B43A, // 60m - amber
        0xFFE04A, // 40m - yellow
        0xA8E04A, // 30m - yellow-green
        0x4CD47A, // 20m - green
        0x36C9A5, // 17m - teal
        0x32C7E8, // 15m - cyan
        0x7B7CFF, // 12m - violet-blue
        0xD85CFF, // 10m - magenta
        0xB64CFF  // 6m - violet
    };

    constexpr const char* SOLAR_BAND_NAMES[] = {
        "160m", "80m", "60m", "40m", "30m", "20m",
        "17m", "15m", "12m", "10m", "6m"
    };

    void centerLabel(lv_obj_t* label, int16_t x, int16_t y, int16_t width)
    {
        lv_obj_set_pos(label, x, y);
        lv_obj_set_width(label, width);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    }

    void createVerticalDivider(lv_obj_t* parent, int16_t x, int16_t y, int16_t height)
    {
        lv_obj_t* divider = lv_obj_create(parent);
        lv_obj_set_pos(divider, x, y);
        lv_obj_set_size(divider, 1, height);
        lv_obj_set_style_bg_color(
            divider, Theme::color(Theme::COLOR_PANEL_BORDER), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(divider, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(divider, 0, LV_PART_MAIN);
        lv_obj_clear_flag(divider, LV_OBJ_FLAG_SCROLLABLE);
    }

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
    lv_obj_set_size(backButton, 116, 30);
    lv_obj_set_style_bg_color(backButton, Theme::color(Theme::COLOR_PANEL), LV_PART_MAIN);
    lv_obj_set_style_border_color(backButton, Theme::color(Theme::COLOR_PANEL_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(backButton, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(backButton, 8, LV_PART_MAIN);
    lv_obj_add_event_cb(backButton, backButtonEventHandler, LV_EVENT_CLICKED, this);
    lv_obj_center(Theme::createLabel(backButton, LV_SYMBOL_LEFT " DASHBOARD", Theme::COLOR_PRIMARY));
    lv_obj_t* title = Theme::createLabel(screen, "SOLAR CONDITIONS DETAIL", Theme::COLOR_PRIMARY);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 140, Theme::CONTENT_TOP + 10);

    lv_obj_t* overview = Theme::createPanel(screen, 8, 110, 252, 328, "HF PROPAGATION");
    conditionLabel = Theme::createLabel(
        overview, "UNKNOWN", Theme::COLOR_SUCCESS, &lv_font_montserrat_28);
    lv_obj_align(conditionLabel, LV_ALIGN_TOP_MID, 0, 30);
    explanationLabel = Theme::createLabel(overview, "WAITING FOR NOAA SWPC", Theme::COLOR_TEXT_MUTED);
    lv_obj_set_width(explanationLabel, 220);
    lv_label_set_long_mode(explanationLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(explanationLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(explanationLabel, LV_ALIGN_TOP_MID, 0, 66);

    bandOutlookLabels[0] = Theme::createLabel(overview, "BAND", Theme::COLOR_PRIMARY);
    bandOutlookLabels[1] = Theme::createLabel(overview, "DAY", Theme::COLOR_PRIMARY);
    bandOutlookLabels[2] = Theme::createLabel(overview, "NIGHT", Theme::COLOR_PRIMARY);
    const int16_t columnX[] = {0, 60, 145};
    const int16_t columnWidth[] = {55, 80, 80};
    for (uint8_t column = 0; column < 3; ++column)
    {
        lv_obj_set_pos(bandOutlookLabels[column], columnX[column], 88);
        lv_obj_set_width(bandOutlookLabels[column], columnWidth[column]);
        lv_obj_set_style_text_align(bandOutlookLabels[column], LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    }

    for (uint8_t index = 0; index < 11; ++index)
    {
        const int16_t y = 113 + index * 17;
        lv_obj_t* badge = lv_obj_create(overview);
        lv_obj_set_pos(badge, 5, y);
        lv_obj_set_size(badge, 48, 16);
        lv_obj_set_style_bg_color(
            badge, Theme::color(SOLAR_BAND_COLORS[index]), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(badge, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(badge, 4, LV_PART_MAIN);
        lv_obj_set_style_pad_all(badge, 0, LV_PART_MAIN);
        lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(badge, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_t* bandLabel = Theme::createLabel(
            badge, SOLAR_BAND_NAMES[index], Theme::COLOR_BACKGROUND,
            &lv_font_montserrat_12);
        lv_obj_center(bandLabel);

        bandDayLabels[index] = Theme::createLabel(
            overview, "--", Theme::COLOR_TEXT, &lv_font_montserrat_12);
        centerLabel(bandDayLabels[index], 60, y, 80);
        bandNightLabels[index] = Theme::createLabel(
            overview, "--", Theme::COLOR_TEXT, &lv_font_montserrat_12);
        centerLabel(bandNightLabels[index], 145, y, 80);
    }

    lv_obj_t* metrics = Theme::createPanel(screen, 268, 110, 252, 328, "LIVE SOLAR METRICS");
    static const char* metricTitles[] = {
        "F10.7 FLUX", "PLANETARY KP", "A INDEX",
        "X-RAY / PEAK", "SOLAR WIND", "B FIELD"
    };
    for (uint8_t index = 0; index < 6; ++index)
    {
        const int16_t column = index % 2;
        const int16_t row = index / 2;
        const int16_t x = column * 116;
        const int16_t y = 36 + row * 90;
        lv_obj_t* metricTitle = Theme::createLabel(
            metrics, metricTitles[index], Theme::COLOR_TEXT_DIM,
            &lv_font_montserrat_12);
        centerLabel(metricTitle, x, y, 116);
        metricValueLabels[index] = Theme::createLabel(
            metrics, "--", Theme::COLOR_TEXT, &lv_font_montserrat_20);
        centerLabel(metricValueLabels[index], x, y + 24, 116);
    }
    createVerticalDivider(metrics, 115, 34, 256);
    Theme::createDivider(metrics, 0, 118, 232);
    Theme::createDivider(metrics, 0, 208, 232);

    lv_obj_t* scales = Theme::createPanel(
        screen, 528, 110, 264, 328, "NOAA SPACE WEATHER SCALES");
    static const char* scaleTitles[] = {
        "RADIO BLACKOUT", "RADIATION STORM", "GEOMAGNETIC STORM"
    };
    for (uint8_t index = 0; index < 3; ++index)
    {
        const int16_t y = 34 + index * 90;
        lv_obj_t* scaleTitle = Theme::createLabel(
            scales, scaleTitles[index], Theme::COLOR_TEXT_DIM,
            &lv_font_montserrat_12);
        centerLabel(scaleTitle, 0, y, 244);
        scaleCodeLabels[index] = Theme::createLabel(
            scales, "--", Theme::COLOR_SUCCESS, &lv_font_montserrat_28);
        centerLabel(scaleCodeLabels[index], 0, y + 17, 244);
        scaleStatusLabels[index] = Theme::createLabel(
            scales, "WAITING", Theme::COLOR_TEXT_MUTED);
        centerLabel(scaleStatusLabels[index], 0, y + 53, 244);
    }
    Theme::createDivider(scales, 0, 116, 244);
    Theme::createDivider(scales, 0, 206, 244);
    update();
    updateTimer = lv_timer_create(updateTimerCallback, 1000, this);
}

void SolarScreen::show() { if (screen != nullptr) { lv_scr_load(screen); update(); } }

void SolarScreen::release()
{
    if (updateTimer != nullptr) { lv_timer_del(updateTimer); updateTimer = nullptr; }
    if (screen != nullptr) { lv_obj_del(screen); screen = nullptr; }
    conditionLabel = explanationLabel = nullptr;
    for (auto& label : bandOutlookLabels) label = nullptr;
    for (auto& label : bandDayLabels) label = nullptr;
    for (auto& label : bandNightLabels) label = nullptr;
    for (auto& label : metricValueLabels) label = nullptr;
    for (auto& label : scaleCodeLabels) label = nullptr;
    for (auto& label : scaleStatusLabels) label = nullptr;
}
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
        lv_obj_clear_flag(explanationLabel, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    const SolarData& data = solarService->getData();
    lv_label_set_text(conditionLabel, solarService->getPropagationLabel());
    const bool disturbed = data.kpIndex >= 4.0f || data.radioBlackoutScale > 0;
    lv_obj_set_style_text_color(conditionLabel,
        Theme::color(disturbed ? Theme::COLOR_WARNING : Theme::COLOR_SUCCESS), LV_PART_MAIN);
    if (disturbed)
    {
        lv_label_set_text(explanationLabel, "DISTURBED HF CONDITIONS");
        lv_obj_clear_flag(explanationLabel, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_add_flag(explanationLabel, LV_OBJ_FLAG_HIDDEN);
    }

    for (uint8_t index = 0; index < 11; ++index)
    {
        lv_label_set_text(
            bandDayLabels[index], ratingLabel(bandRating(index, true, data)));
        lv_label_set_text(
            bandNightLabels[index], ratingLabel(bandRating(index, false, data)));
    }

    const String metricValues[] = {
        String(data.solarFlux, 0) + " SFU",
        String(data.kpIndex, 1),
        String(data.aIndex),
        data.xrayClass + " / " + data.flarePeakClass,
        String(data.solarWindSpeed, 0) + " KM/S",
        String(data.magneticField, 1) + " NT"
    };
    for (uint8_t index = 0; index < 6; ++index)
        lv_label_set_text(metricValueLabels[index], metricValues[index].c_str());

    const int scaleValues[] = {
        data.radioBlackoutScale,
        data.solarRadiationScale,
        data.geomagneticStormScale
    };
    const char scaleLetters[] = {'R', 'S', 'G'};
    for (uint8_t index = 0; index < 3; ++index)
    {
        const String code = String(scaleLetters[index]) + String(scaleValues[index]);
        lv_label_set_text(scaleCodeLabels[index], code.c_str());
        lv_label_set_text(scaleStatusLabels[index],
            scaleValues[index] == 0 ? "NONE" : "ACTIVE");
        lv_obj_set_style_text_color(
            scaleCodeLabels[index],
            Theme::color(scaleValues[index] == 0
                ? Theme::COLOR_SUCCESS : Theme::COLOR_WARNING),
            LV_PART_MAIN);
    }
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
