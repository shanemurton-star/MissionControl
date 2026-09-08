#include "LiveSpotsScreen.h"
#include "../services/NetworkUpdateState.h"
#include <WiFi.h>
#include "../ui/Theme.h"
#include "../ui/WorldMapAsset.h"

namespace
{
    constexpr int16_t WORLD_MAP_X = 0;
    constexpr int16_t WORLD_MAP_Y = 31;
    constexpr uint16_t WORLD_MAP_ZOOM = 188;
    constexpr int16_t WORLD_MAP_WIDTH = 376;
    constexpr int16_t WORLD_MAP_HEIGHT = 188;

    // HamClock-inspired progression across the HF bands: warm colors on the
    // lower bands transitioning through green/cyan to violet on 10 meters.
    constexpr uint32_t BAND_COLORS[LiveSpotsService::BAND_COUNT] = {
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

    void styleMapPrimitive(lv_obj_t* object)
    {
        lv_obj_set_style_pad_all(object, 0, LV_PART_MAIN);
        lv_obj_clear_flag(object, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(object, LV_OBJ_FLAG_CLICKABLE);
    }

    int16_t longitudeToMapX(float longitude)
    {
        const float bounded = constrain(longitude, -180.0f, 180.0f);
        return WORLD_MAP_X + static_cast<int16_t>(roundf(
            (bounded + 180.0f) / 360.0f * (WORLD_MAP_WIDTH - 1)));
    }

    int16_t latitudeToMapY(float latitude)
    {
        const float bounded = constrain(latitude, -90.0f, 90.0f);
        return WORLD_MAP_Y + static_cast<int16_t>(roundf(
            (90.0f - bounded) / 180.0f * (WORLD_MAP_HEIGHT - 1)));
    }

    enum class SignalRegion : uint8_t
    {
        NorthAmerica,
        SouthAmerica,
        Europe,
        Africa,
        Asia,
        Oceania,
        Other
    };

    SignalRegion signalRegion(float latitude, float longitude)
    {
        if (latitude >= 7.0f && latitude <= 84.0f &&
            longitude >= -170.0f && longitude <= -50.0f)
            return SignalRegion::NorthAmerica;
        if (latitude >= -60.0f && latitude <= 15.0f &&
            longitude >= -90.0f && longitude <= -30.0f)
            return SignalRegion::SouthAmerica;
        if (latitude >= 35.0f && latitude <= 72.0f &&
            longitude >= -25.0f && longitude <= 45.0f)
            return SignalRegion::Europe;
        if (latitude >= -37.0f && latitude <= 37.0f &&
            longitude >= -20.0f && longitude <= 55.0f)
            return SignalRegion::Africa;
        if (latitude >= 5.0f && latitude <= 80.0f &&
            longitude >= 26.0f && longitude <= 180.0f)
            return SignalRegion::Asia;
        if (latitude >= -50.0f && latitude <= 10.0f &&
            longitude >= 105.0f && longitude <= 180.0f)
            return SignalRegion::Oceania;
        return SignalRegion::Other;
    }
}

void LiveSpotsScreen::begin(ClockService& clockService, LiveSpotsService& spotsService)
{
    if (screen != nullptr) return;
    service = &spotsService;
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
    titleLabel = Theme::createLabel(screen, "LIVE PSK SPOTS", Theme::COLOR_PRIMARY);
    lv_obj_align(titleLabel, LV_ALIGN_TOP_LEFT, 140, Theme::CONTENT_TOP + 10);
    lv_obj_set_width(titleLabel, 280);
    lv_label_set_long_mode(titleLabel, LV_LABEL_LONG_CLIP);

    lv_obj_t* signalReachButton = lv_btn_create(screen);
    lv_obj_set_pos(signalReachButton, 426, Theme::CONTENT_TOP);
    lv_obj_set_size(signalReachButton, 174, 30);
    lv_obj_set_style_bg_color(
        signalReachButton, Theme::color(Theme::COLOR_PANEL), LV_PART_MAIN);
    lv_obj_set_style_border_color(
        signalReachButton, Theme::color(Theme::COLOR_PANEL_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(signalReachButton, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(signalReachButton, 8, LV_PART_MAIN);
    lv_obj_add_event_cb(
        signalReachButton, signalReachButtonEventHandler, LV_EVENT_CLICKED, this);
    lv_obj_center(Theme::createLabel(
        signalReachButton, "SIGNAL REACH", Theme::COLOR_PRIMARY,
        &lv_font_montserrat_12));

    lv_obj_t* lookupButton = lv_btn_create(screen);
    lv_obj_set_pos(lookupButton, 608, Theme::CONTENT_TOP);
    lv_obj_set_size(lookupButton, 184, 30);
    lv_obj_set_style_bg_color(
        lookupButton, Theme::color(Theme::COLOR_PANEL), LV_PART_MAIN);
    lv_obj_set_style_border_color(
        lookupButton, Theme::color(Theme::COLOR_PANEL_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(lookupButton, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(lookupButton, 8, LV_PART_MAIN);
    lv_obj_add_event_cb(
        lookupButton, lookupButtonEventHandler, LV_EVENT_CLICKED, this);
    lv_obj_center(Theme::createLabel(
        lookupButton, "CONTACT LOOKUP", Theme::COLOR_PRIMARY,
        &lv_font_montserrat_12));

    lv_obj_t* bandPanel = Theme::createPanel(screen, 8, 110, 380, 328, "BAND ACTIVITY  |  LAST HOUR");
    lv_obj_t* bandHeading = Theme::createLabel(
        bandPanel, "BAND", Theme::COLOR_TEXT_DIM);
    lv_obj_set_pos(bandHeading, 0, 28);
    lv_obj_t* activityHeading = Theme::createLabel(
        bandPanel, "ACTIVITY", Theme::COLOR_TEXT_DIM);
    lv_obj_set_pos(activityHeading, 50, 28);
    lv_obj_t* spotsHeading = Theme::createLabel(
        bandPanel, "SPOTS", Theme::COLOR_TEXT_DIM);
    lv_obj_set_pos(spotsHeading, 172, 28);
    lv_obj_t* farthestHeading = Theme::createLabel(
        bandPanel, "FARTHEST", Theme::COLOR_TEXT_DIM);
    lv_obj_set_pos(farthestHeading, 226, 28);
    for (uint8_t index = 0; index < LiveSpotsService::BAND_COUNT; ++index)
    {
        const int16_t rowY = 52 + index * 27;
        lv_obj_t* bandLabel = Theme::createLabel(
            bandPanel, spotsService.getBand(index).name, Theme::COLOR_TEXT);
        lv_obj_set_pos(bandLabel, 0, rowY - 5);
        lv_obj_set_width(bandLabel, 42);
        lv_obj_set_style_text_color(
            bandLabel, Theme::color(BAND_COLORS[index]), LV_PART_MAIN);

        lv_obj_t* track = lv_obj_create(bandPanel);
        lv_obj_set_pos(track, 50, rowY);
        lv_obj_set_size(track, 112, 8);
        lv_obj_set_style_bg_color(
            track, Theme::color(Theme::COLOR_PANEL_BORDER), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(track, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(track, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(track, 4, LV_PART_MAIN);
        lv_obj_set_style_pad_all(track, 0, LV_PART_MAIN);
        lv_obj_clear_flag(track, LV_OBJ_FLAG_SCROLLABLE);

        bandBars[index] = lv_obj_create(track);
        lv_obj_set_pos(bandBars[index], 0, 0);
        lv_obj_set_size(bandBars[index], 1, 8);
        lv_obj_set_style_bg_color(
            bandBars[index], Theme::color(BAND_COLORS[index]), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(bandBars[index], LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(bandBars[index], 0, LV_PART_MAIN);
        lv_obj_set_style_radius(bandBars[index], 4, LV_PART_MAIN);
        lv_obj_set_style_pad_all(bandBars[index], 0, LV_PART_MAIN);
        lv_obj_clear_flag(bandBars[index], LV_OBJ_FLAG_SCROLLABLE);

        bandCountLabels[index] = Theme::createLabel(
            bandPanel, "0", Theme::COLOR_TEXT_MUTED);
        lv_obj_set_pos(bandCountLabels[index], 172, rowY - 5);
        lv_obj_set_width(bandCountLabels[index], 40);
        lv_obj_set_style_text_align(
            bandCountLabels[index], LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);

        bandFarthestLabels[index] = Theme::createLabel(
            bandPanel, "--", Theme::COLOR_TEXT_DIM, &lv_font_montserrat_12);
        lv_obj_set_pos(bandFarthestLabels[index], 226, rowY - 3);
        lv_obj_set_size(bandFarthestLabels[index], 132, 16);
        lv_label_set_long_mode(bandFarthestLabels[index], LV_LABEL_LONG_CLIP);
    }
    lv_obj_t* signalPanel = Theme::createPanel(
        screen, 396, 110, 396, 328, "MY SIGNAL REACH  |  LAST 30 MIN");

    lv_obj_t* mapImage = lv_img_create(signalPanel);
    lv_img_set_src(mapImage, &WorldMapAsset::IMAGE);
    lv_img_set_pivot(mapImage, 0, 0);
    lv_img_set_zoom(mapImage, WORLD_MAP_ZOOM);
    lv_obj_set_pos(mapImage, WORLD_MAP_X, WORLD_MAP_Y);
    lv_obj_clear_flag(mapImage, LV_OBJ_FLAG_CLICKABLE);

    signalHomeMarker = lv_obj_create(signalPanel);
    lv_obj_set_size(signalHomeMarker, 9, 9);
    lv_obj_set_style_bg_color(
        signalHomeMarker, Theme::color(Theme::COLOR_TEXT), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(signalHomeMarker, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(
        signalHomeMarker, Theme::color(Theme::COLOR_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_border_width(signalHomeMarker, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(signalHomeMarker, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    styleMapPrimitive(signalHomeMarker);
    const int16_t homeX = longitudeToMapX(
        static_cast<float>(spotsService.getHomeLongitude()));
    const int16_t homeY = latitudeToMapY(
        static_cast<float>(spotsService.getHomeLatitude()));
    lv_obj_set_pos(signalHomeMarker, homeX - 4, homeY - 4);

    for (uint8_t index = 0;
         index < LiveSpotsService::MAX_SIGNAL_SPOTS;
         ++index)
    {
        signalDots[index] = lv_obj_create(signalPanel);
        lv_obj_set_size(signalDots[index], 7, 7);
        lv_obj_set_style_border_width(signalDots[index], 0, LV_PART_MAIN);
        lv_obj_set_style_radius(
            signalDots[index], LV_RADIUS_CIRCLE, LV_PART_MAIN);
        styleMapPrimitive(signalDots[index]);
        lv_obj_add_flag(signalDots[index], LV_OBJ_FLAG_HIDDEN);
    }

    signalRegionsLabel = Theme::createLabel(
        signalPanel, "NA 0   EU 0   AS 0   SA 0   AF 0   OC 0",
        Theme::COLOR_TEXT_DIM, &lv_font_montserrat_12);
    lv_obj_set_pos(signalRegionsLabel, 0, 229);
    lv_obj_set_width(signalRegionsLabel, 376);
    lv_obj_set_style_text_align(
        signalRegionsLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    signalSummaryLabel = Theme::createLabel(
        signalPanel, "WAITING FOR YOUR SIGNAL", Theme::COLOR_TEXT_MUTED,
        &lv_font_montserrat_12);
    lv_obj_set_pos(signalSummaryLabel, 0, 251);
    lv_obj_set_width(signalSummaryLabel, 376);
    lv_obj_set_height(signalSummaryLabel, 34);
    lv_label_set_long_mode(signalSummaryLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(
        signalSummaryLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    signalRefreshLabel = Theme::createLabel(
        signalPanel, "NEXT REFRESH PENDING", Theme::COLOR_TEXT_DIM,
        &lv_font_montserrat_12);
    lv_obj_set_pos(signalRefreshLabel, 0, 291);
    lv_obj_set_width(signalRefreshLabel, 376);
    lv_obj_set_style_text_align(
        signalRefreshLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    statusLabel = Theme::createLabel(
        signalPanel, "WAITING FOR PSK REPORTER", Theme::COLOR_WARNING);
    lv_obj_set_pos(statusLabel, 50, 132);
    lv_obj_set_width(statusLabel, 276);
    lv_obj_set_style_text_align(statusLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    update();
    updateTimer = lv_timer_create(updateTimerCallback, 1000, this);
}

void LiveSpotsScreen::show() { if (screen != nullptr) { lv_scr_load(screen); update(); } }

void LiveSpotsScreen::release()
{
    if (updateTimer != nullptr) { lv_timer_del(updateTimer); updateTimer = nullptr; }
    if (screen != nullptr) { lv_obj_del(screen); screen = nullptr; }
    statusLabel = signalHomeMarker = signalRegionsLabel =
        signalSummaryLabel = signalRefreshLabel = titleLabel = nullptr;
    for (uint8_t index = 0; index < LiveSpotsService::BAND_COUNT; ++index)
        bandBars[index] = bandCountLabels[index] = bandFarthestLabels[index] = nullptr;
    for (auto& dot : signalDots) dot = nullptr;
    renderedDataRevision = renderedAgeMinute = UINT32_MAX;
}
void LiveSpotsScreen::setNavigationCallback(NavigationCallback callback) { navigationCallback = callback; }

void LiveSpotsScreen::update()
{
    headerBar.update();
    if (NetworkUpdateState::isBusy()) return;
    if (service == nullptr) return;
    const String title = "LIVE PSK SPOTS  |  " + service->getGridSquare();
    lv_label_set_text(titleLabel, title.c_str());
    if (!service->isValid())
    {
        lv_obj_clear_flag(statusLabel, LV_OBJ_FLAG_HIDDEN);
        if (WiFi.status() != WL_CONNECTED) lv_label_set_text(statusLabel, "WAITING FOR WIFI");
        else if (service->isUpdating()) lv_label_set_text(statusLabel, "UPDATING LIVE SPOTS...");
        else lv_label_set_text(statusLabel, service->getLastError().c_str());
        return;
    }
    lv_obj_add_flag(statusLabel, LV_OBJ_FLAG_HIDDEN);
    const uint32_t ageMinute = static_cast<uint32_t>(time(nullptr) / 60);
    const uint32_t dataRevision = service->getDataRevision();
    if (dataRevision == renderedDataRevision && ageMinute == renderedAgeMinute)
        return;
    renderedDataRevision = dataRevision;
    renderedAgeMinute = ageMinute;

    const uint32_t refreshSeconds = service->getSecondsUntilNextUpdate();
    if (refreshSeconds == 0)
    {
        lv_label_set_text(signalRefreshLabel, "NEXT REFRESH DUE");
    }
    else
    {
        const time_t refreshTime = time(nullptr) + refreshSeconds;
        struct tm localRefresh = {};
        localtime_r(&refreshTime, &localRefresh);
        char refreshText[32];
        strftime(refreshText, sizeof(refreshText),
            "NEXT REFRESH %H:%M LOCAL", &localRefresh);
        lv_label_set_text(signalRefreshLabel, refreshText);
    }

    uint16_t maximumCount = 0;
    for (uint8_t i = 0; i < LiveSpotsService::BAND_COUNT; ++i)
        maximumCount = max(maximumCount, service->getBand(i).count);
    for (uint8_t i = 0; i < LiveSpotsService::BAND_COUNT; ++i)
    {
        const BandSpotSummary& band = service->getBand(i);
        const int16_t barWidth = band.count == 0 || maximumCount == 0
            ? 1
            : max(4, static_cast<int>(112UL * band.count / maximumCount));
        lv_obj_set_width(bandBars[i], barWidth);
        lv_obj_set_style_bg_opa(
            bandBars[i], band.count == 0 ? LV_OPA_TRANSP : LV_OPA_COVER,
            LV_PART_MAIN);
        lv_label_set_text(bandCountLabels[i], String(band.count).c_str());
        String farthest = "--";
        if (band.count > 0)
            farthest = String(band.farthestKm, 0) + "km " + band.farthestCall;
        lv_label_set_text(bandFarthestLabels[i], farthest.c_str());
    }

    const time_t now = time(nullptr);
    for (auto* dot : signalDots)
        lv_obj_add_flag(dot, LV_OBJ_FLAG_HIDDEN);

    if (!service->isSignalReachValid())
    {
        lv_label_set_text(statusLabel,
            service->getSignalReachError().isEmpty()
                ? "UPDATING MY SIGNAL REACH"
                : service->getSignalReachError().c_str());
        lv_obj_clear_flag(statusLabel, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(signalSummaryLabel, "");
        return;
    }
    lv_obj_add_flag(statusLabel, LV_OBJ_FLAG_HIDDEN);

    const uint8_t count = service->getSignalReachCount();
    if (count == 0)
    {
        lv_label_set_text(
            signalRegionsLabel, "NA 0   EU 0   AS 0   SA 0   AF 0   OC 0");
        const String idle = "NO " + service->getCallsign() +
            " REPORTS IN LAST 30 MIN  |  TRANSMIT FT8 TO SEE YOUR REACH";
        lv_label_set_text(signalSummaryLabel, idle.c_str());
        return;
    }

    uint8_t northAmerica = 0;
    uint8_t southAmerica = 0;
    uint8_t europe = 0;
    uint8_t africa = 0;
    uint8_t asia = 0;
    uint8_t oceania = 0;
    for (uint8_t index = 0; index < count; ++index)
    {
        const SignalReachSpot& spot = service->getSignalReachSpot(index);
        const int16_t x = longitudeToMapX(spot.longitude);
        const int16_t y = latitudeToMapY(spot.latitude);
        const int16_t size = spot.snr >= -5 ? 9 : (spot.snr >= -15 ? 7 : 5);
        lv_obj_set_pos(signalDots[index], x - size / 2, y - size / 2);
        lv_obj_set_size(signalDots[index], size, size);
        lv_obj_set_style_bg_color(
            signalDots[index], Theme::color(BAND_COLORS[spot.bandIndex]),
            LV_PART_MAIN);
        const uint32_t ageSeconds = now > static_cast<time_t>(spot.timestamp)
            ? static_cast<uint32_t>(now - spot.timestamp)
            : 0;
        const lv_opa_t opacity = ageSeconds < 5UL * 60UL
            ? LV_OPA_COVER
            : (ageSeconds < 15UL * 60UL ? LV_OPA_70 : LV_OPA_40);
        lv_obj_set_style_bg_opa(signalDots[index], opacity, LV_PART_MAIN);
        const bool farthest = fabsf(
            spot.distanceKm - service->getSignalFarthestKm()) < 1.0f;
        lv_obj_set_style_border_width(signalDots[index], farthest ? 2 : 0,
            LV_PART_MAIN);
        lv_obj_set_style_border_color(
            signalDots[index], Theme::color(Theme::COLOR_TEXT), LV_PART_MAIN);
        lv_obj_clear_flag(signalDots[index], LV_OBJ_FLAG_HIDDEN);

        switch (signalRegion(spot.latitude, spot.longitude))
        {
            case SignalRegion::NorthAmerica: ++northAmerica; break;
            case SignalRegion::SouthAmerica: ++southAmerica; break;
            case SignalRegion::Europe: ++europe; break;
            case SignalRegion::Africa: ++africa; break;
            case SignalRegion::Asia: ++asia; break;
            case SignalRegion::Oceania: ++oceania; break;
            case SignalRegion::Other: break;
        }
    }

    const String regions = "NA " + String(northAmerica) +
        "   EU " + String(europe) + "   AS " + String(asia) +
        "   SA " + String(southAmerica) + "   AF " + String(africa) +
        "   OC " + String(oceania);
    lv_label_set_text(signalRegionsLabel, regions.c_str());

    String summary = String(count) + " RECEIVERS  |  " +
        service->getSignalReportCount() + " REPORTS  |  MAX " +
        String(service->getSignalFarthestKm(), 0) + " KM";
    if (!service->getSignalFarthestCall().isEmpty())
        summary += "  " + service->getSignalFarthestCall();
    lv_label_set_text(signalSummaryLabel, summary.c_str());
}

void LiveSpotsScreen::backButtonEventHandler(lv_event_t* event)
{
    LiveSpotsScreen* self = static_cast<LiveSpotsScreen*>(lv_event_get_user_data(event));
    if (self != nullptr && self->navigationCallback != nullptr) self->navigationCallback(Page::Dashboard);
}
void LiveSpotsScreen::lookupButtonEventHandler(lv_event_t* event)
{
    LiveSpotsScreen* self = static_cast<LiveSpotsScreen*>(lv_event_get_user_data(event));
    if (self != nullptr && self->navigationCallback != nullptr)
        self->navigationCallback(Page::CallsignLookup);
}
void LiveSpotsScreen::signalReachButtonEventHandler(lv_event_t* event)
{
    LiveSpotsScreen* self = static_cast<LiveSpotsScreen*>(lv_event_get_user_data(event));
    if (self != nullptr && self->navigationCallback != nullptr)
        self->navigationCallback(Page::SignalReach);
}
void LiveSpotsScreen::updateTimerCallback(lv_timer_t* timer)
{
    LiveSpotsScreen* self = static_cast<LiveSpotsScreen*>(timer->user_data);
    if (self != nullptr) self->update();
}
