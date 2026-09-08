#include "PotaScreen.h"
#include "../services/NetworkUpdateState.h"

#include <WiFi.h>

#include "../ui/Theme.h"

namespace
{
    constexpr uint32_t POTA_PLOT_COLOR = 0xB34DFF;
    constexpr int16_t MAP_CENTER_X = PotaService::MAP_VIEW_WIDTH / 2;
    constexpr int16_t MAP_CENTER_Y = PotaService::MAP_VIEW_HEIGHT / 2;

    void styleMapOverlay(lv_obj_t* object)
    {
        lv_obj_set_style_pad_all(object, 0, LV_PART_MAIN);
        lv_obj_clear_flag(object, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(object, LV_OBJ_FLAG_CLICKABLE);
    }

    String formatFrequency(const String& rawFrequency)
    {
        const float value = rawFrequency.toFloat();
        if (value <= 0.0f) return "FREQ --";
        const float megahertz = value >= 1000.0f ? value / 1000.0f : value;
        return String(megahertz, 3) + " MHz";
    }
}

void PotaScreen::begin(ClockService& clockService, PotaService& service)
{
    if (screen != nullptr) return;
    potaService = &service;
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

    lv_obj_t* title = Theme::createLabel(screen, "POTA SPOTS DETAIL", Theme::COLOR_PRIMARY);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 140, Theme::CONTENT_TOP + 10);
    statusLabel = Theme::createLabel(screen, "", Theme::COLOR_TEXT_MUTED);
    lv_obj_align(statusLabel, LV_ALIGN_TOP_RIGHT, -10, Theme::CONTENT_TOP + 10);

    lv_obj_t* left = Theme::createPanel(
        screen, 8, 110, 388, 328, "ACTIVE PARKS WITHIN 100 MILES");
    lv_obj_add_flag(left, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(left, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(left, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_t* right = Theme::createPanel(
        screen, 404, 110, 388, 328, "ACTIVE PARK MAP  |  100 MI RADIUS");
    activeListLabel = Theme::createLabel(
        left, "WAITING FOR POTA", Theme::COLOR_TEXT_MUTED);
    lv_obj_set_pos(activeListLabel, 0, 32);
    lv_obj_set_width(activeListLabel, 354);

    mapViewport = lv_obj_create(right);
    lv_obj_set_pos(mapViewport, 0, 31);
    lv_obj_set_size(
        mapViewport, PotaService::MAP_VIEW_WIDTH, PotaService::MAP_VIEW_HEIGHT);
    lv_obj_set_style_bg_color(
        mapViewport, Theme::color(Theme::COLOR_BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(mapViewport, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(mapViewport, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(mapViewport, 4, LV_PART_MAIN);
    styleMapOverlay(mapViewport);

    for (uint8_t index = 0; index < PotaService::MAP_TILE_COUNT; ++index)
    {
        mapTileImages[index] = lv_img_create(mapViewport);
        styleMapOverlay(mapTileImages[index]);
        lv_obj_add_flag(mapTileImages[index], LV_OBJ_FLAG_HIDDEN);
    }

    const int16_t rangeRadius = static_cast<int16_t>(roundf(
        PotaService::ACTIVE_RADIUS_MILES /
        max(0.1f, potaService->getMapMilesPerPixel())));
    lv_obj_t* rangeRing = lv_obj_create(mapViewport);
    lv_obj_set_pos(
        rangeRing, MAP_CENTER_X - rangeRadius, MAP_CENTER_Y - rangeRadius);
    lv_obj_set_size(rangeRing, rangeRadius * 2, rangeRadius * 2);
    lv_obj_set_style_bg_opa(rangeRing, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_color(
        rangeRing, Theme::color(POTA_PLOT_COLOR), LV_PART_MAIN);
    lv_obj_set_style_border_opa(rangeRing, LV_OPA_80, LV_PART_MAIN);
    lv_obj_set_style_border_width(rangeRing, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(rangeRing, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    styleMapOverlay(rangeRing);

    lv_obj_t* home = lv_obj_create(mapViewport);
    lv_obj_set_pos(home, MAP_CENTER_X - 5, MAP_CENTER_Y - 5);
    lv_obj_set_size(home, 10, 10);
    lv_obj_set_style_bg_color(
        home, Theme::color(Theme::COLOR_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(home, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(home, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(home, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_pad_all(home, 0, LV_PART_MAIN);
    styleMapOverlay(home);
    lv_obj_t* homeLabel = Theme::createLabel(
        mapViewport, "HOME", Theme::COLOR_PRIMARY, &lv_font_montserrat_12);
    lv_obj_set_pos(homeLabel, MAP_CENTER_X + 7, MAP_CENTER_Y - 9);

    for (uint8_t index = 0; index < PotaService::MAX_SPOTS; ++index)
    {
        plotMarkers[index] = lv_obj_create(mapViewport);
        lv_obj_set_size(plotMarkers[index], 8, 8);
        lv_obj_set_style_bg_color(
            plotMarkers[index], Theme::color(POTA_PLOT_COLOR), LV_PART_MAIN);
        lv_obj_set_style_border_width(plotMarkers[index], 0, LV_PART_MAIN);
        lv_obj_set_style_radius(
            plotMarkers[index], LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_pad_all(plotMarkers[index], 0, LV_PART_MAIN);
        styleMapOverlay(plotMarkers[index]);
        lv_obj_add_flag(plotMarkers[index], LV_OBJ_FLAG_HIDDEN);
    }

    lv_obj_t* attribution = Theme::createLabel(
        mapViewport, "(c) OpenStreetMap contributors", Theme::COLOR_TEXT,
        &lv_font_montserrat_12);
    lv_obj_align(attribution, LV_ALIGN_BOTTOM_RIGHT, -4, -2);
    lv_obj_set_style_bg_color(
        attribution, Theme::color(Theme::COLOR_BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(attribution, LV_OPA_70, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(attribution, 3, LV_PART_MAIN);

    mapStatusLabel = Theme::createLabel(
        mapViewport, "LOADING REGIONAL MAP...", Theme::COLOR_TEXT,
        &lv_font_montserrat_14);
    lv_obj_set_width(mapStatusLabel, PotaService::MAP_VIEW_WIDTH - 20);
    lv_obj_align(mapStatusLabel, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_align(
        mapStatusLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(
        mapStatusLabel, Theme::color(Theme::COLOR_BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(mapStatusLabel, LV_OPA_70, LV_PART_MAIN);
    lv_obj_set_style_pad_all(mapStatusLabel, 5, LV_PART_MAIN);

    potaService->requestMap();

    update();
    updateTimer = lv_timer_create(updateTimerCallback, 1000, this);
}

void PotaScreen::show()
{
    if (screen != nullptr) { lv_scr_load(screen); update(); }
}

void PotaScreen::release()
{
    if (updateTimer != nullptr) { lv_timer_del(updateTimer); updateTimer = nullptr; }
    for (uint8_t index = 0; index < PotaService::MAP_TILE_COUNT; ++index)
        if (mapTileDescriptors[index].data != nullptr)
            lv_img_cache_invalidate_src(&mapTileDescriptors[index]);
    if (screen != nullptr) { lv_obj_del(screen); screen = nullptr; }
    statusLabel = activeListLabel = mapViewport = mapStatusLabel = nullptr;
    for (uint8_t index = 0; index < PotaService::MAP_TILE_COUNT; ++index)
        mapTileImages[index] = nullptr;
    for (uint8_t index = 0; index < PotaService::MAX_SPOTS; ++index)
        plotMarkers[index] = nullptr;
    renderedDataRevision = UINT32_MAX;
    renderedMapGeneration = UINT32_MAX;
}

void PotaScreen::setNavigationCallback(NavigationCallback callback)
{
    navigationCallback = callback;
}

void PotaScreen::update()
{
    headerBar.update();
    if (NetworkUpdateState::isBusy()) return;
    if (potaService == nullptr) return;
    const uint32_t mapGeneration = potaService->getMapGeneration();
    if (mapGeneration != renderedMapGeneration)
    {
        renderedMapGeneration = mapGeneration;
        for (uint8_t index = 0;
             index < potaService->getMapTileCount(); ++index)
        {
            const uint8_t* tileData = potaService->getMapTileData(index);
            if (tileData == nullptr || potaService->getMapTileSize(index) == 0)
                continue;
            if (mapTileDescriptors[index].data != nullptr)
                lv_img_cache_invalidate_src(&mapTileDescriptors[index]);
            mapTileDescriptors[index].header.always_zero = 0;
            mapTileDescriptors[index].header.cf = LV_IMG_CF_RAW_ALPHA;
            mapTileDescriptors[index].header.w = 256;
            mapTileDescriptors[index].header.h = 256;
            mapTileDescriptors[index].data = tileData;
            mapTileDescriptors[index].data_size =
                potaService->getMapTileSize(index);
            lv_img_set_src(mapTileImages[index], &mapTileDescriptors[index]);
            lv_obj_set_pos(
                mapTileImages[index], potaService->getMapTileX(index),
                potaService->getMapTileY(index));
            lv_obj_clear_flag(mapTileImages[index], LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (mapGeneration > 0)
    {
        lv_obj_add_flag(mapStatusLabel, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_clear_flag(mapStatusLabel, LV_OBJ_FLAG_HIDDEN);
        if (potaService->isMapLoading())
            lv_label_set_text(mapStatusLabel, "LOADING REGIONAL MAP...");
        else if (!potaService->getMapError().isEmpty())
            lv_label_set_text(mapStatusLabel, potaService->getMapError().c_str());
        else
            lv_label_set_text(mapStatusLabel, "REGIONAL MAP UNAVAILABLE");
    }

    if (!potaService->isValid())
    {
        const char* status = WiFi.status() != WL_CONNECTED ? "WAITING FOR WIFI" :
            (potaService->isUpdating() ? "UPDATING POTA SPOTS..." : potaService->getLastError().c_str());
        lv_label_set_text(activeListLabel, status);
        return;
    }

    const uint32_t dataRevision = potaService->getDataRevision();
    if (dataRevision == renderedDataRevision) return;
    renderedDataRevision = dataRevision;

    const uint8_t count = potaService->getSpotCount();
    lv_label_set_text(statusLabel, (String(count) + " ACTIVE PARKS  |  15 MIN REFRESH").c_str());
    String list;
    for (uint8_t index = 0; index < potaService->getSpotCount(); ++index)
    {
        const PotaSpotData& spot = potaService->getSpot(index);
        String parkName = spot.name;
        if (parkName.length() > 30) parkName = parkName.substring(0, 30);
        if (!list.isEmpty()) list += "\n\n";
        list += spot.reference + "   " + String(spot.distanceMiles, 0) + " mi\n";
        list += parkName + "\n" + spot.activator + "  |  ";
        list += spot.mode.isEmpty() ? "MODE --" : spot.mode;
        list += "  |  " + formatFrequency(spot.frequency);
    }
    if (list.isEmpty()) list = "NO ACTIVE PARKS WITHIN 100 MILES";
    lv_label_set_text(activeListLabel, list.c_str());

    for (uint8_t index = 0; index < PotaService::MAX_SPOTS; ++index)
    {
        if (index >= count)
        {
            lv_obj_add_flag(plotMarkers[index], LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        const PotaSpotData& park = potaService->getSpot(index);
        int16_t markerX = 0;
        int16_t markerY = 0;
        if (!potaService->projectToMap(
                park.latitude, park.longitude, markerX, markerY))
        {
            lv_obj_add_flag(plotMarkers[index], LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        lv_obj_set_pos(plotMarkers[index], markerX - 4, markerY - 4);
        lv_obj_set_style_bg_opa(plotMarkers[index], LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_color(
            plotMarkers[index], Theme::color(Theme::COLOR_TEXT), LV_PART_MAIN);
        lv_obj_set_style_border_width(plotMarkers[index], 1, LV_PART_MAIN);
        lv_obj_clear_flag(plotMarkers[index], LV_OBJ_FLAG_HIDDEN);
    }
}

void PotaScreen::backButtonEventHandler(lv_event_t* event)
{
    PotaScreen* self = static_cast<PotaScreen*>(lv_event_get_user_data(event));
    if (self != nullptr && self->navigationCallback != nullptr) self->navigationCallback(Page::Dashboard);
}

void PotaScreen::updateTimerCallback(lv_timer_t* timer)
{
    PotaScreen* self = static_cast<PotaScreen*>(timer->user_data);
    if (self != nullptr) self->update();
}
