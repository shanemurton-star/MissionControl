#include "SignalReachScreen.h"

#include <WiFi.h>

#include "../services/NetworkUpdateState.h"
#include "../ui/Theme.h"
#include "../ui/WorldMapAsset.h"

namespace
{
    constexpr int16_t MAP_X = 0;
    constexpr int16_t MAP_Y = 31;
    constexpr uint16_t MAP_ZOOM = 256;
    constexpr int16_t MAP_WIDTH =
        static_cast<int16_t>(WorldMapAsset::WIDTH * MAP_ZOOM / 256);
    constexpr int16_t MAP_HEIGHT =
        static_cast<int16_t>(WorldMapAsset::HEIGHT * MAP_ZOOM / 256);

    constexpr uint32_t BAND_COLORS[LiveSpotsService::BAND_COUNT] = {
        0xFF8C32, 0xE8B43A, 0xFFE04A, 0xA8E04A, 0x4CD47A,
        0x36C9A5, 0x32C7E8, 0x7B7CFF, 0xD85CFF, 0xB64CFF
    };

    void styleButton(lv_obj_t* button)
    {
        lv_obj_set_style_bg_color(
            button, Theme::color(Theme::COLOR_PANEL), LV_PART_MAIN);
        lv_obj_set_style_border_color(
            button, Theme::color(Theme::COLOR_PANEL_BORDER), LV_PART_MAIN);
        lv_obj_set_style_border_width(button, 1, LV_PART_MAIN);
        lv_obj_set_style_radius(button, 8, LV_PART_MAIN);
    }

    void styleMapPrimitive(lv_obj_t* object)
    {
        lv_obj_set_style_pad_all(object, 0, LV_PART_MAIN);
        lv_obj_clear_flag(object, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(object, LV_OBJ_FLAG_CLICKABLE);
    }

    int16_t longitudeToMapX(float longitude)
    {
        const float bounded = constrain(longitude, -180.0f, 180.0f);
        return MAP_X + static_cast<int16_t>(roundf(
            (bounded + 180.0f) / 360.0f * (MAP_WIDTH - 1)));
    }

    int16_t latitudeToMapY(float latitude)
    {
        const float bounded = constrain(latitude, -90.0f, 90.0f);
        return MAP_Y + static_cast<int16_t>(roundf(
            (90.0f - bounded) / 180.0f * (MAP_HEIGHT - 1)));
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

void SignalReachScreen::begin(
    ClockService& clockService,
    LiveSpotsService& spotsService)
{
    if (screen != nullptr) return;
    service = &spotsService;

    screen = lv_obj_create(nullptr);
    Theme::configureScreen(screen);
    headerBar.create(
        screen, clockService, Theme::SCREEN_WIDTH, Theme::HEADER_HEIGHT);
    headerBar.setSettingsCallback([this]() {
        if (navigationCallback != nullptr) navigationCallback(Page::Settings);
    });
    headerBar.setNavigationCallback([this](Page page) {
        if (navigationCallback != nullptr) navigationCallback(page);
    });

    lv_obj_t* backButton = lv_btn_create(screen);
    lv_obj_set_pos(backButton, 8, Theme::CONTENT_TOP);
    lv_obj_set_size(backButton, 116, 30);
    styleButton(backButton);
    lv_obj_add_event_cb(
        backButton, backButtonEventHandler, LV_EVENT_CLICKED, this);
    lv_obj_center(Theme::createLabel(
        backButton, LV_SYMBOL_LEFT " LIVE SPOTS", Theme::COLOR_PRIMARY,
        &lv_font_montserrat_12));

    titleLabel = Theme::createLabel(
        screen, "MY SIGNAL REACH", Theme::COLOR_PRIMARY);
    lv_obj_set_pos(titleLabel, 140, Theme::CONTENT_TOP + 8);
    lv_obj_set_width(titleLabel, 650);

    lv_obj_t* mapPanel = Theme::createPanel(
        screen, 8, 110, 784, 328, "RECEPTION MAP  |  LAST 30 MINUTES");

    lv_obj_t* mapImage = lv_img_create(mapPanel);
    lv_img_set_src(mapImage, &WorldMapAsset::IMAGE);
    lv_img_set_pivot(mapImage, 0, 0);
    lv_img_set_zoom(mapImage, MAP_ZOOM);
    lv_obj_set_pos(mapImage, MAP_X, MAP_Y);
    lv_obj_clear_flag(mapImage, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t* verticalDivider = lv_obj_create(mapPanel);
    lv_obj_set_pos(verticalDivider, 532, 30);
    lv_obj_set_size(verticalDivider, 1, 268);
    lv_obj_set_style_bg_color(
        verticalDivider, Theme::color(Theme::COLOR_PANEL_BORDER), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(verticalDivider, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(verticalDivider, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(verticalDivider, 0, LV_PART_MAIN);
    lv_obj_clear_flag(verticalDivider, LV_OBJ_FLAG_SCROLLABLE);

    homeMarker = lv_obj_create(mapPanel);
    lv_obj_set_size(homeMarker, 12, 12);
    lv_obj_set_style_bg_color(
        homeMarker, Theme::color(Theme::COLOR_TEXT), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(homeMarker, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(
        homeMarker, Theme::color(Theme::COLOR_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_border_width(homeMarker, 3, LV_PART_MAIN);
    lv_obj_set_style_radius(homeMarker, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    styleMapPrimitive(homeMarker);
    const int16_t homeX = longitudeToMapX(
        static_cast<float>(spotsService.getHomeLongitude()));
    const int16_t homeY = latitudeToMapY(
        static_cast<float>(spotsService.getHomeLatitude()));
    lv_obj_set_pos(homeMarker, homeX - 6, homeY - 6);

    for (uint8_t index = 0;
         index < LiveSpotsService::MAX_SIGNAL_SPOTS;
         ++index)
    {
        signalDots[index] = lv_obj_create(mapPanel);
        lv_obj_set_size(signalDots[index], 8, 8);
        lv_obj_set_style_border_width(signalDots[index], 0, LV_PART_MAIN);
        lv_obj_set_style_radius(
            signalDots[index], LV_RADIUS_CIRCLE, LV_PART_MAIN);
        styleMapPrimitive(signalDots[index]);
        lv_obj_add_flag(signalDots[index], LV_OBJ_FLAG_HIDDEN);
    }

    statusLabel = Theme::createLabel(
        mapPanel, "WAITING FOR PSK REPORTER", Theme::COLOR_WARNING);
    lv_obj_set_pos(statusLabel, 80, 156);
    lv_obj_set_width(statusLabel, 370);
    lv_obj_set_style_text_align(statusLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    receiverCountLabel = Theme::createLabel(
        mapPanel, "--", Theme::COLOR_TEXT, &lv_font_montserrat_28);
    lv_obj_set_pos(receiverCountLabel, 548, 40);
    lv_obj_set_width(receiverCountLabel, 196);
    lv_obj_set_style_text_align(
        receiverCountLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_t* receiversCaption = Theme::createLabel(
        mapPanel, "RECEIVERS", Theme::COLOR_TEXT_DIM, &lv_font_montserrat_12);
    lv_obj_set_pos(receiversCaption, 548, 75);
    lv_obj_set_width(receiversCaption, 196);
    lv_obj_set_style_text_align(
        receiversCaption, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    reportCountLabel = Theme::createLabel(
        mapPanel, "-- REPORTS", Theme::COLOR_TEXT_MUTED,
        &lv_font_montserrat_14);
    lv_obj_set_pos(reportCountLabel, 548, 100);
    lv_obj_set_width(reportCountLabel, 196);
    lv_obj_set_style_text_align(
        reportCountLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    Theme::createDivider(mapPanel, 548, 127, 196);
    lv_obj_t* farthestCaption = Theme::createLabel(
        mapPanel, "FARTHEST RECEPTION", Theme::COLOR_TEXT_DIM,
        &lv_font_montserrat_12);
    lv_obj_set_pos(farthestCaption, 548, 140);
    lv_obj_set_width(farthestCaption, 196);
    lv_obj_set_style_text_align(
        farthestCaption, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    farthestLabel = Theme::createLabel(
        mapPanel, "--", Theme::COLOR_PRIMARY, &lv_font_montserrat_20);
    lv_obj_set_pos(farthestLabel, 548, 162);
    lv_obj_set_size(farthestLabel, 196, 48);
    lv_label_set_long_mode(farthestLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(
        farthestLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    lv_obj_t* regionsCaption = Theme::createLabel(
        mapPanel, "REGIONS", Theme::COLOR_TEXT_DIM, &lv_font_montserrat_12);
    lv_obj_set_pos(regionsCaption, 548, 215);
    lv_obj_set_width(regionsCaption, 196);
    lv_obj_set_style_text_align(
        regionsCaption, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    regionsLabel = Theme::createLabel(
        mapPanel, "NA 0   EU 0   AS 0\nSA 0   AF 0   OC 0",
        Theme::COLOR_TEXT_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(regionsLabel, 548, 236);
    lv_obj_set_size(regionsLabel, 196, 36);
    lv_obj_set_style_text_align(
        regionsLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    emptyLabel = Theme::createLabel(
        mapPanel, "", Theme::COLOR_TEXT_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(emptyLabel, 12, 291);
    lv_obj_set_width(emptyLabel, 508);
    lv_obj_set_height(emptyLabel, 15);
    lv_label_set_long_mode(emptyLabel, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(emptyLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    refreshLabel = Theme::createLabel(
        mapPanel, "NEXT REFRESH PENDING", Theme::COLOR_TEXT_DIM,
        &lv_font_montserrat_12);
    lv_obj_set_pos(refreshLabel, 548, 286);
    lv_obj_set_width(refreshLabel, 196);
    lv_obj_set_style_text_align(refreshLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    update();
    updateTimer = lv_timer_create(updateTimerCallback, 1000, this);
}

void SignalReachScreen::show()
{
    if (screen != nullptr) { lv_scr_load(screen); update(); }
}

void SignalReachScreen::release()
{
    if (updateTimer != nullptr) { lv_timer_del(updateTimer); updateTimer = nullptr; }
    if (screen != nullptr) { lv_obj_del(screen); screen = nullptr; }
    titleLabel = statusLabel = homeMarker = receiverCountLabel =
        reportCountLabel = farthestLabel = regionsLabel = emptyLabel =
        refreshLabel = nullptr;
    for (auto& dot : signalDots) dot = nullptr;
    renderedDataRevision = renderedAgeMinute = UINT32_MAX;
}

void SignalReachScreen::setNavigationCallback(NavigationCallback callback)
{
    navigationCallback = callback;
}

void SignalReachScreen::update()
{
    headerBar.update();
    if (NetworkUpdateState::isBusy() || service == nullptr) return;

    const String title = "MY SIGNAL REACH  |  " + service->getCallsign();
    lv_label_set_text(titleLabel, title.c_str());

    const uint32_t ageMinute = static_cast<uint32_t>(time(nullptr) / 60);
    const uint32_t dataRevision = service->getDataRevision();
    if (dataRevision == renderedDataRevision && ageMinute == renderedAgeMinute)
        return;
    renderedDataRevision = dataRevision;
    renderedAgeMinute = ageMinute;

    const uint32_t refreshSeconds = service->getSecondsUntilNextUpdate();
    if (refreshSeconds == 0)
    {
        lv_label_set_text(refreshLabel, "NEXT REFRESH DUE");
    }
    else
    {
        const time_t refreshTime = time(nullptr) + refreshSeconds;
        struct tm localRefresh = {};
        localtime_r(&refreshTime, &localRefresh);
        char refreshText[32];
        strftime(refreshText, sizeof(refreshText), "REFRESH %H:%M LOCAL", &localRefresh);
        lv_label_set_text(refreshLabel, refreshText);
    }

    for (auto* dot : signalDots)
        lv_obj_add_flag(dot, LV_OBJ_FLAG_HIDDEN);

    if (!service->isSignalReachValid())
    {
        lv_obj_clear_flag(statusLabel, LV_OBJ_FLAG_HIDDEN);
        if (WiFi.status() != WL_CONNECTED)
            lv_label_set_text(statusLabel, "WAITING FOR WIFI");
        else if (service->isUpdating())
            lv_label_set_text(statusLabel, "UPDATING MY SIGNAL REACH...");
        else
            lv_label_set_text(statusLabel,
                service->getSignalReachError().isEmpty()
                    ? "WAITING FOR SIGNAL DATA"
                    : service->getSignalReachError().c_str());
        lv_label_set_text(receiverCountLabel, "--");
        lv_label_set_text(reportCountLabel, "-- REPORTS");
        lv_label_set_text(farthestLabel, "--");
        lv_label_set_text(emptyLabel, "");
        return;
    }
    lv_obj_add_flag(statusLabel, LV_OBJ_FLAG_HIDDEN);

    const uint8_t count = service->getSignalReachCount();
    lv_label_set_text(receiverCountLabel, String(count).c_str());
    lv_label_set_text(
        reportCountLabel,
        (String(service->getSignalReportCount()) + " REPORTS").c_str());

    if (count == 0)
    {
        lv_label_set_text(farthestLabel, "--");
        lv_label_set_text(regionsLabel, "NA 0   EU 0   AS 0\nSA 0   AF 0   OC 0");
        const String idle = "NO " + service->getCallsign() +
            " REPORTS IN THE LAST 30 MINUTES  |  TRANSMIT FT8 TO SEE YOUR REACH";
        lv_label_set_text(emptyLabel, idle.c_str());
        return;
    }

    lv_label_set_text(emptyLabel, "DOT COLOR = BAND  |  BRIGHTNESS = REPORT AGE");
    const String farthest = service->getSignalFarthestCall() + "\n" +
        String(service->getSignalFarthestKm(), 0) + " KM";
    lv_label_set_text(farthestLabel, farthest.c_str());

    uint8_t northAmerica = 0;
    uint8_t southAmerica = 0;
    uint8_t europe = 0;
    uint8_t africa = 0;
    uint8_t asia = 0;
    uint8_t oceania = 0;
    const time_t now = time(nullptr);
    for (uint8_t index = 0; index < count; ++index)
    {
        const SignalReachSpot& spot = service->getSignalReachSpot(index);
        const int16_t x = longitudeToMapX(spot.longitude);
        const int16_t y = latitudeToMapY(spot.latitude);
        const int16_t size = spot.snr >= -5 ? 12 : (spot.snr >= -15 ? 9 : 7);
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
        const bool isFarthest = fabsf(
            spot.distanceKm - service->getSignalFarthestKm()) < 1.0f;
        lv_obj_set_style_border_width(
            signalDots[index], isFarthest ? 2 : 0, LV_PART_MAIN);
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
        "\nSA " + String(southAmerica) + "   AF " + String(africa) +
        "   OC " + String(oceania);
    lv_label_set_text(regionsLabel, regions.c_str());
}

void SignalReachScreen::backButtonEventHandler(lv_event_t* event)
{
    SignalReachScreen* self =
        static_cast<SignalReachScreen*>(lv_event_get_user_data(event));
    if (self != nullptr && self->navigationCallback != nullptr)
        self->navigationCallback(Page::LiveSpots);
}

void SignalReachScreen::updateTimerCallback(lv_timer_t* timer)
{
    SignalReachScreen* self = static_cast<SignalReachScreen*>(timer->user_data);
    if (self != nullptr) self->update();
}
