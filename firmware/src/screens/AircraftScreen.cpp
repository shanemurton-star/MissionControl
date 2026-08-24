#include "AircraftScreen.h"
#include "../services/NetworkUpdateState.h"

#include <WiFi.h>
#include <math.h>
#include <time.h>
#include "../ui/Theme.h"

namespace
{
    lv_obj_t* createRadarReferenceLine(
        lv_obj_t* parent, int16_t width, int16_t height,
        int16_t xOffset, int16_t yOffset, lv_opa_t opacity)
    {
        lv_obj_t* line = lv_obj_create(parent);
        lv_obj_set_size(line, width, height);
        lv_obj_align(line, LV_ALIGN_CENTER, xOffset, yOffset);
        lv_obj_set_style_bg_color(
            line, Theme::color(Theme::COLOR_PANEL_BORDER), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(line, opacity, LV_PART_MAIN);
        lv_obj_set_style_border_width(line, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(line, 0, LV_PART_MAIN);
        lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(line, LV_OBJ_FLAG_CLICKABLE);
        return line;
    }

}

void AircraftScreen::begin(ClockService& clockService, AircraftService& service)
{
    if (screen != nullptr) return;
    aircraftService = &service;
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

    lv_obj_t* title = Theme::createLabel(screen, "LIVE AIRCRAFT DETAIL", Theme::COLOR_PRIMARY);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 140, Theme::CONTENT_TOP + 10);
    countLabel = Theme::createLabel(screen, "-- AIRCRAFT WITHIN 25 NM", Theme::COLOR_TEXT);
    lv_obj_align(countLabel, LV_ALIGN_TOP_RIGHT, -8, Theme::CONTENT_TOP + 10);

    radarScope = Theme::createPanel(screen, 8, 110, 500, 328, "AIRSPACE  |  25 NM RANGE");
    lv_obj_t* scope = radarScope;

    // Geographic reference grid. It is deliberately procedural so the radar
    // remains useful at any configured location without downloading map tiles.
    createRadarReferenceLine(scope, 290, 1, 0, 10, LV_OPA_30);
    createRadarReferenceLine(scope, 1, 290, 0, 10, LV_OPA_30);
    createRadarReferenceLine(scope, 252, 1, 0, -62, LV_OPA_20);
    createRadarReferenceLine(scope, 252, 1, 0, 82, LV_OPA_20);
    createRadarReferenceLine(scope, 1, 252, -72, 10, LV_OPA_20);
    createRadarReferenceLine(scope, 1, 252, 72, 10, LV_OPA_20);

    const int16_t ringSizes[4] = {74, 146, 218, 290};
    for (int16_t size : ringSizes)
    {
        lv_obj_t* ring = lv_obj_create(scope);
        lv_obj_set_size(ring, size, size);
        lv_obj_align(ring, LV_ALIGN_CENTER, 0, 10);
        lv_obj_set_style_radius(ring, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(ring, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(ring, 1, LV_PART_MAIN);
        lv_obj_set_style_border_color(ring, Theme::color(Theme::COLOR_PANEL_BORDER), LV_PART_MAIN);
        lv_obj_set_style_border_opa(ring, LV_OPA_60, LV_PART_MAIN);
        lv_obj_clear_flag(ring, LV_OBJ_FLAG_SCROLLABLE);
    }
    lv_obj_t* home = lv_obj_create(scope);
    lv_obj_set_size(home, 9, 9);
    lv_obj_align(home, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_radius(home, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(home, Theme::color(Theme::COLOR_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_border_width(home, 0, LV_PART_MAIN);
    lv_obj_clear_flag(home, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t* north = Theme::createLabel(scope, "N", Theme::COLOR_TEXT_DIM);
    lv_obj_align(north, LV_ALIGN_TOP_MID, 0, 25);
    lv_obj_t* south = Theme::createLabel(scope, "S", Theme::COLOR_TEXT_DIM);
    lv_obj_align(south, LV_ALIGN_BOTTOM_MID, 0, -1);
    lv_obj_t* east = Theme::createLabel(scope, "E", Theme::COLOR_TEXT_DIM);
    lv_obj_align(east, LV_ALIGN_CENTER, 160, 10);
    lv_obj_t* west = Theme::createLabel(scope, "W", Theme::COLOR_TEXT_DIM);
    lv_obj_align(west, LV_ALIGN_CENTER, -160, 10);

    const char* rangeText[4] = {"6", "12", "19", "25 NM"};
    const int16_t rangeOffset[4] = {26, 52, 77, 103};
    for (uint8_t index = 0; index < 4; ++index)
    {
        lv_obj_t* range = Theme::createLabel(
            scope, rangeText[index], Theme::COLOR_TEXT_DIM,
            &lv_font_montserrat_14);
        lv_obj_align(
            range, LV_ALIGN_CENTER, rangeOffset[index],
            10 + rangeOffset[index]);
    }

    radarPositionLabel = Theme::createLabel(
        scope, "HOME", Theme::COLOR_TEXT_DIM, &lv_font_montserrat_14);
    lv_obj_align(radarPositionLabel, LV_ALIGN_CENTER, 0, 28);

    for (uint8_t i = 0; i < AircraftService::MAX_AIRCRAFT; ++i)
    {
        targetLeaderLines[i] = lv_line_create(scope);
        lv_obj_set_size(targetLeaderLines[i], 500, 328);
        lv_obj_set_pos(targetLeaderLines[i], 0, 0);
        lv_obj_set_style_line_color(
            targetLeaderLines[i], Theme::color(Theme::COLOR_TEXT_DIM), LV_PART_MAIN);
        lv_obj_set_style_line_width(targetLeaderLines[i], 1, LV_PART_MAIN);
        lv_obj_set_style_line_opa(targetLeaderLines[i], LV_OPA_60, LV_PART_MAIN);
        lv_obj_clear_flag(targetLeaderLines[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(targetLeaderLines[i], LV_OBJ_FLAG_HIDDEN);

        targets[i] = lv_obj_create(scope);
        lv_obj_set_size(targets[i], 7, 7);
        lv_obj_set_style_radius(targets[i], LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_bg_color(targets[i], Theme::color(Theme::COLOR_SUCCESS), LV_PART_MAIN);
        lv_obj_set_style_border_width(targets[i], 0, LV_PART_MAIN);
        lv_obj_clear_flag(targets[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(targets[i], LV_OBJ_FLAG_HIDDEN);
        targetLabels[i] = Theme::createLabel(scope, "", Theme::COLOR_TEXT_MUTED, &lv_font_montserrat_14);
        lv_obj_add_flag(targetLabels[i], LV_OBJ_FLAG_HIDDEN);
    }

    lv_obj_t* listPanel = Theme::createPanel(
        screen, 516, 110, 276, 328, "NEAREST AIRCRAFT");
    statusLabel = Theme::createLabel(listPanel, "WAITING FOR LIVE ADS-B", Theme::COLOR_WARNING);
    lv_obj_align(statusLabel, LV_ALIGN_TOP_LEFT, 0, 30);

    aircraftListScroller = lv_obj_create(listPanel);
    lv_obj_set_pos(aircraftListScroller, 0, 28);
    lv_obj_set_size(aircraftListScroller, 254, 280);
    lv_obj_set_style_bg_opa(aircraftListScroller, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(aircraftListScroller, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(aircraftListScroller, 0, LV_PART_MAIN);
    lv_obj_set_scroll_dir(aircraftListScroller, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(aircraftListScroller, LV_SCROLLBAR_MODE_AUTO);

    for (uint8_t index = 0; index < AircraftService::MAX_AIRCRAFT; ++index)
    {
        aircraftRowButtons[index] = lv_btn_create(aircraftListScroller);
        lv_obj_set_pos(aircraftRowButtons[index], 0, index * 46);
        lv_obj_set_size(aircraftRowButtons[index], 238, 46);
        lv_obj_set_style_bg_opa(
            aircraftRowButtons[index], LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_bg_color(
            aircraftRowButtons[index], Theme::color(Theme::COLOR_PRIMARY),
            LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(
            aircraftRowButtons[index], LV_OPA_20,
            LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_color(
            aircraftRowButtons[index], Theme::color(Theme::COLOR_PANEL_BORDER), LV_PART_MAIN);
        lv_obj_set_style_border_width(aircraftRowButtons[index], 0, LV_PART_MAIN);
        lv_obj_set_style_outline_width(aircraftRowButtons[index], 0, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(aircraftRowButtons[index], 0, LV_PART_MAIN);
        lv_obj_set_style_radius(aircraftRowButtons[index], 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(aircraftRowButtons[index], 4, LV_PART_MAIN);
        lv_obj_add_event_cb(
            aircraftRowButtons[index], aircraftRowEventHandler, LV_EVENT_CLICKED, this);
        aircraftRowLabels[index] = Theme::createLabel(
            aircraftRowButtons[index], "", Theme::COLOR_TEXT_MUTED);
        lv_obj_set_width(aircraftRowLabels[index], 226);
        lv_obj_align(aircraftRowLabels[index], LV_ALIGN_LEFT_MID, 0, 0);

        aircraftRowSeparators[index] = lv_obj_create(aircraftListScroller);
        lv_obj_set_pos(aircraftRowSeparators[index], 0, index * 46 + 45);
        lv_obj_set_size(aircraftRowSeparators[index], 238, 1);
        lv_obj_set_style_bg_color(
            aircraftRowSeparators[index],
            Theme::color(Theme::COLOR_PANEL_BORDER), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(
            aircraftRowSeparators[index], LV_OPA_70, LV_PART_MAIN);
        lv_obj_set_style_border_width(
            aircraftRowSeparators[index], 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(
            aircraftRowSeparators[index], 0, LV_PART_MAIN);
        lv_obj_clear_flag(
            aircraftRowSeparators[index], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(
            aircraftRowSeparators[index], LV_OBJ_FLAG_CLICKABLE);

        lv_obj_add_flag(aircraftRowButtons[index], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(aircraftRowSeparators[index], LV_OBJ_FLAG_HIDDEN);
    }

    detailScreen = lv_obj_create(nullptr);
    Theme::configureScreen(detailScreen);
    detailHeaderBar.create(detailScreen, clockService, Theme::SCREEN_WIDTH, Theme::HEADER_HEIGHT);
    detailHeaderBar.setSettingsCallback([this]() {
        if (navigationCallback != nullptr) navigationCallback(Page::Settings);
    });
    detailHeaderBar.setNavigationCallback([this](Page page) {
        if (navigationCallback != nullptr) navigationCallback(page);
    });
    lv_obj_t* detailBackButton = lv_btn_create(detailScreen);
    lv_obj_set_pos(detailBackButton, 8, Theme::CONTENT_TOP);
    lv_obj_set_size(detailBackButton, 132, 30);
    lv_obj_set_style_bg_color(detailBackButton, Theme::color(Theme::COLOR_PANEL), LV_PART_MAIN);
    lv_obj_set_style_border_color(detailBackButton, Theme::color(Theme::COLOR_PANEL_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(detailBackButton, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(detailBackButton, 8, LV_PART_MAIN);
    lv_obj_add_event_cb(detailBackButton, detailBackButtonEventHandler, LV_EVENT_CLICKED, this);
    lv_obj_center(Theme::createLabel(detailBackButton, LV_SYMBOL_LEFT " AIRCRAFT", Theme::COLOR_PRIMARY));
    lv_obj_t* detailTitle = Theme::createLabel(detailScreen, "AIRCRAFT INFORMATION", Theme::COLOR_PRIMARY);
    lv_obj_align(detailTitle, LV_ALIGN_TOP_LEFT, 156, Theme::CONTENT_TOP + 10);

    lv_obj_t* identityPanel = Theme::createPanel(detailScreen, 8, 110, 310, 328, "");
    detailIdentityLabel = Theme::createLabel(
        identityPanel, "--", Theme::COLOR_TEXT, &lv_font_montserrat_28);
    lv_obj_set_pos(detailIdentityLabel, 0, 4);
    lv_obj_set_width(detailIdentityLabel, 282);
    lv_obj_set_style_text_align(detailIdentityLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    detailFlightLabel = Theme::createLabel(
        identityPanel, "AIRCRAFT TYPE UNAVAILABLE", Theme::COLOR_TEXT_MUTED,
        &lv_font_montserrat_20);
    lv_obj_set_pos(detailFlightLabel, 0, 44);
    lv_obj_set_width(detailFlightLabel, 282);
    lv_obj_set_height(detailFlightLabel, 48);
    lv_label_set_long_mode(detailFlightLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(detailFlightLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    detailArtworkImage = lv_img_create(identityPanel);
    lv_obj_clear_flag(detailArtworkImage, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(detailArtworkImage, LV_OBJ_FLAG_HIDDEN);

    detailArtworkStatusLabel = Theme::createLabel(
        identityPanel, "", Theme::COLOR_TEXT_DIM, &lv_font_montserrat_14);
    lv_obj_set_pos(detailArtworkStatusLabel, 0, 276);
    lv_obj_set_width(detailArtworkStatusLabel, 282);
    lv_obj_set_style_text_align(
        detailArtworkStatusLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    lv_obj_t* dataPanel = Theme::createPanel(
        detailScreen, 326, 110, 466, 328, "FLIGHT AT A GLANCE");
    const char* placeholders[7] = {
        "ALTITUDE\n-- ft", "GROUND SPEED\n-- kt",
        "DISTANCE\n-- nm", "BEARING\n-- deg",
        "LATITUDE\n--", "LONGITUDE\n--", "EMERGENCY  --"
    };
    for (uint8_t index = 0; index < 6; ++index)
    {
        detailMetricLabels[index] = Theme::createLabel(
            dataPanel, placeholders[index], Theme::COLOR_TEXT);
        lv_obj_set_pos(
            detailMetricLabels[index], (index % 2) * 220, 38 + (index / 2) * 78);
        lv_obj_set_size(detailMetricLabels[index], 214, 64);
        lv_obj_set_style_text_align(
            detailMetricLabels[index], LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    }
    detailMetricLabels[6] = Theme::createLabel(
        dataPanel, placeholders[6], Theme::COLOR_SUCCESS);
    lv_obj_set_pos(detailMetricLabels[6], 0, 274);
    lv_obj_set_width(detailMetricLabels[6], 438);
    lv_obj_set_style_text_align(
        detailMetricLabels[6], LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    detailPositionLabel = detailMetricLabels[6];

    update();
    updateTimer = lv_timer_create(updateTimerCallback, 1000, this);
}

void AircraftScreen::show() { if (screen != nullptr) { lv_scr_load(screen); update(); } }

void AircraftScreen::release()
{
    if (updateTimer != nullptr) { lv_timer_del(updateTimer); updateTimer = nullptr; }
    if (detailScreen != nullptr) { lv_obj_del(detailScreen); detailScreen = nullptr; }
    if (screen != nullptr) { lv_obj_del(screen); screen = nullptr; }
    countLabel = statusLabel = listLabel = aircraftListScroller = nullptr;
    radarScope = radarPositionLabel = nullptr;
    detailIdentityLabel = detailFlightLabel = detailPositionLabel = nullptr;
    detailArtworkImage = nullptr;
    detailArtworkStatusLabel = nullptr;
    for (uint8_t i = 0; i < 7; ++i) detailMetricLabels[i] = nullptr;
    selectedAircraftHex = "";
    displayedArtworkHex = "";
    for (uint8_t i = 0; i < AircraftService::MAX_AIRCRAFT; ++i)
    {
        aircraftRowButtons[i] = aircraftRowLabels[i] = nullptr;
        aircraftRowSeparators[i] = nullptr;
        targets[i] = targetLabels[i] = targetLeaderLines[i] = nullptr;
    }
}
void AircraftScreen::setNavigationCallback(NavigationCallback callback) { navigationCallback = callback; }

void AircraftScreen::update()
{
    headerBar.update();
    if (NetworkUpdateState::isBusy()) return;
    if (aircraftService == nullptr) return;
    const uint8_t count = aircraftService->getAircraftCount();
    String countText = String(count) + " AIRCRAFT WITHIN 25 NM";
    lv_label_set_text(countLabel, countText.c_str());
    if (radarPositionLabel != nullptr)
    {
        const double centerLat = aircraftService->getCenterLatitude();
        const double centerLon = aircraftService->getCenterLongitude();
        String position = "HOME  " + String(fabs(centerLat), 3) +
            (centerLat >= 0.0 ? " N  " : " S  ") +
            String(fabs(centerLon), 3) + (centerLon >= 0.0 ? " E" : " W");
        lv_label_set_text(radarPositionLabel, position.c_str());
    }

    if (!aircraftService->isValid())
    {
        lv_obj_add_flag(aircraftListScroller, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(statusLabel, LV_OBJ_FLAG_HIDDEN);
        if (WiFi.status() != WL_CONNECTED) lv_label_set_text(statusLabel, "WAITING FOR WIFI");
        else if (aircraftService->isUpdating()) lv_label_set_text(statusLabel, "UPDATING LIVE TRAFFIC...");
        else lv_label_set_text(statusLabel, aircraftService->getLastError().c_str());
        return;
    }
    lv_obj_add_flag(statusLabel, LV_OBJ_FLAG_HIDDEN);
    if (count > 0) lv_obj_clear_flag(aircraftListScroller, LV_OBJ_FLAG_HIDDEN);

    const uint8_t rows = count;
    for (uint8_t i = 0; i < rows; ++i)
    {
        const AircraftData& item = aircraftService->getAircraft(i);
        String name = !item.callsign.isEmpty() ? item.callsign :
            (!item.registration.isEmpty() ? item.registration : item.hex);
        String row = name + "   " + String(item.distanceNm, 1) + " nm\n";
        row += item.onGround ? "GROUND" : String(static_cast<int>(item.altitudeFeet)) + " ft";
        row += "   " + String(static_cast<int>(item.groundSpeedKnots)) + " kt";
        lv_label_set_text(aircraftRowLabels[i], row.c_str());
        lv_obj_clear_flag(aircraftRowButtons[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(aircraftRowSeparators[i], LV_OBJ_FLAG_HIDDEN);
    }
    for (uint8_t i = rows; i < AircraftService::MAX_AIRCRAFT; ++i)
    {
        lv_obj_add_flag(aircraftRowButtons[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(aircraftRowSeparators[i], LV_OBJ_FLAG_HIDDEN);
    }
    if (count == 0)
    {
        lv_obj_add_flag(aircraftListScroller, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(statusLabel, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(statusLabel, "NO POSITIONED AIRCRAFT\nCURRENTLY WITHIN RANGE");
    }

    for (uint8_t i = 0; i < AircraftService::MAX_AIRCRAFT; ++i)
    {
        if (i >= count)
        {
            lv_obj_add_flag(targets[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(targetLabels[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(targetLeaderLines[i], LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        const AircraftData& item = aircraftService->getAircraft(i);
        const float radians = item.bearingDegrees * DEG_TO_RAD;
        const float radius = (item.distanceNm / AircraftService::SEARCH_RADIUS_NM) * 145.0f;
        const int16_t x = static_cast<int16_t>(sin(radians) * radius);
        const int16_t y = static_cast<int16_t>(-cos(radians) * radius) + 10;
        lv_obj_align(targets[i], LV_ALIGN_CENTER, x, y);
        lv_obj_clear_flag(targets[i], LV_OBJ_FLAG_HIDDEN);
        String shortName = !item.callsign.isEmpty() ? item.callsign : item.hex;
        lv_label_set_text(targetLabels[i], shortName.c_str());
    }

    // Assign callsigns to balanced left/right columns, then pack each column
    // vertically. Aircraft dots remain at their true bearing/range positions;
    // leader lines make each label association unambiguous.
    int16_t dotX[AircraftService::MAX_AIRCRAFT] = {};
    int16_t dotY[AircraftService::MAX_AIRCRAFT] = {};
    int8_t side[AircraftService::MAX_AIRCRAFT] = {};
    uint8_t columnOrder[2][AircraftService::MAX_AIRCRAFT] = {};
    uint8_t columnCount[2] = {0, 0};

    for (uint8_t i = 0; i < count; ++i)
    {
        const AircraftData& item = aircraftService->getAircraft(i);
        const float radians = item.bearingDegrees * DEG_TO_RAD;
        const float radius =
            (item.distanceNm / AircraftService::SEARCH_RADIUS_NM) * 145.0f;
        dotX[i] = static_cast<int16_t>(sin(radians) * radius);
        dotY[i] = static_cast<int16_t>(-cos(radians) * radius) + 10;

        uint8_t column = dotX[i] < 0 ? 0 : 1;
        if (columnCount[column] >= 12) column = 1 - column;
        side[i] = column == 0 ? -1 : 1;
        columnOrder[column][columnCount[column]++] = i;
    }

    constexpr int16_t LABEL_TOP = -122;
    constexpr int16_t LABEL_BOTTOM = 142;
    constexpr int16_t LABEL_SPACING = 22;
    constexpr int16_t LABEL_X = 178;

    for (uint8_t column = 0; column < 2; ++column)
    {
        // Sort this column by the aircraft's actual vertical position.
        for (uint8_t i = 1; i < columnCount[column]; ++i)
        {
            const uint8_t value = columnOrder[column][i];
            int8_t position = static_cast<int8_t>(i) - 1;
            while (position >= 0 &&
                   dotY[columnOrder[column][position]] > dotY[value])
            {
                columnOrder[column][position + 1] =
                    columnOrder[column][position];
                --position;
            }
            columnOrder[column][position + 1] = value;
        }

        int16_t labelY[AircraftService::MAX_AIRCRAFT] = {};
        for (uint8_t position = 0; position < columnCount[column]; ++position)
        {
            const uint8_t aircraftIndex = columnOrder[column][position];
            int16_t y = constrain(dotY[aircraftIndex], LABEL_TOP, LABEL_BOTTOM);
            if (position > 0 && y < labelY[position - 1] + LABEL_SPACING)
                y = labelY[position - 1] + LABEL_SPACING;
            labelY[position] = y;
        }

        if (columnCount[column] > 0)
        {
            const int16_t overflow =
                labelY[columnCount[column] - 1] - LABEL_BOTTOM;
            if (overflow > 0)
                for (uint8_t position = 0; position < columnCount[column]; ++position)
                    labelY[position] -= overflow;
        }

        for (uint8_t position = 0; position < columnCount[column]; ++position)
        {
            const uint8_t aircraftIndex = columnOrder[column][position];
            const int16_t labelX = side[aircraftIndex] * LABEL_X;
            lv_obj_align(
                targetLabels[aircraftIndex], LV_ALIGN_CENTER,
                labelX, labelY[position]);
            lv_obj_clear_flag(
                targetLabels[aircraftIndex], LV_OBJ_FLAG_HIDDEN);
        }
    }

    // Resolve all layout changes once, then use actual rendered coordinates
    // instead of assuming the panel has no padding. Each leader touches the
    // marker edge and the nearest callsign edge.
    lv_obj_update_layout(radarScope);
    for (uint8_t aircraftIndex = 0; aircraftIndex < count; ++aircraftIndex)
    {
        lv_area_t dotArea;
        lv_area_t labelArea;
        lv_area_t lineArea;
        lv_obj_get_coords(targets[aircraftIndex], &dotArea);
        lv_obj_get_coords(targetLabels[aircraftIndex], &labelArea);
        lv_obj_get_coords(targetLeaderLines[aircraftIndex], &lineArea);
        const float dotCenterX = (dotArea.x1 + dotArea.x2) * 0.5f;
        const float dotCenterY = (dotArea.y1 + dotArea.y2) * 0.5f;
        const float labelAnchorX = side[aircraftIndex] < 0
            ? labelArea.x2 + 2.0f : labelArea.x1 - 2.0f;
        const float labelAnchorY = (labelArea.y1 + labelArea.y2) * 0.5f;
        const float deltaX = labelAnchorX - dotCenterX;
        const float deltaY = labelAnchorY - dotCenterY;
        const float length = sqrtf(deltaX * deltaX + deltaY * deltaY);
        const float markerRadius = 5.0f;
        const float dotAnchorX = length > 0.1f
            ? dotCenterX + deltaX * markerRadius / length : dotCenterX;
        const float dotAnchorY = length > 0.1f
            ? dotCenterY + deltaY * markerRadius / length : dotCenterY;

        targetLeaderPoints[aircraftIndex][0] = {
            static_cast<lv_coord_t>(dotAnchorX - lineArea.x1),
            static_cast<lv_coord_t>(dotAnchorY - lineArea.y1)};
        targetLeaderPoints[aircraftIndex][1] = {
            static_cast<lv_coord_t>(labelAnchorX - lineArea.x1),
            static_cast<lv_coord_t>(labelAnchorY - lineArea.y1)};
        lv_line_set_points(
            targetLeaderLines[aircraftIndex],
            targetLeaderPoints[aircraftIndex], 2);
        lv_obj_clear_flag(
            targetLeaderLines[aircraftIndex], LV_OBJ_FLAG_HIDDEN);
    }

    if (lv_scr_act() == detailScreen) updateDetail();
}

void AircraftScreen::showAircraftDetail(uint8_t index)
{
    if (aircraftService == nullptr || index >= aircraftService->getAircraftCount()) return;
    selectedAircraftHex = aircraftService->getAircraft(index).hex;
    displayedArtworkHex = "";
    updateDetail();
    lv_scr_load(detailScreen);
}

void AircraftScreen::updateDetail()
{
    detailHeaderBar.update();
    if (aircraftService == nullptr || selectedAircraftHex.isEmpty()) return;
    const AircraftData* selected = nullptr;
    for (uint8_t index = 0; index < aircraftService->getAircraftCount(); ++index)
        if (aircraftService->getAircraft(index).hex == selectedAircraftHex)
            selected = &aircraftService->getAircraft(index);
    if (selected == nullptr)
    {
        lv_obj_set_style_text_font(
            detailIdentityLabel, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_label_set_text(detailIdentityLabel, "AIRCRAFT NO LONGER WITHIN RANGE");
        return;
    }

    lv_obj_set_style_text_font(
        detailIdentityLabel, &lv_font_montserrat_28, LV_PART_MAIN);

    const String tailNumber = !selected->registration.isEmpty()
        ? selected->registration
        : (!selected->callsign.isEmpty() ? selected->callsign : selected->hex);
    const String aircraftType = !selected->description.isEmpty()
        ? selected->description
        : (!selected->type.isEmpty() ? selected->type : "AIRCRAFT TYPE UNAVAILABLE");
    lv_label_set_text(detailIdentityLabel, tailNumber.c_str());
    lv_label_set_text(detailFlightLabel, aircraftType.c_str());
    updateAircraftArtwork();

    const String altitude = selected->onGround
        ? "ALTITUDE\nON GROUND"
        : "ALTITUDE\n" + String(selected->altitudeFeet, 0) + " ft";
    const String speed = "GROUND SPEED\n" +
        String(selected->groundSpeedKnots, 0) + " kt";
    const String distance = "DISTANCE\n" + String(selected->distanceNm, 1) + " nm";
    const String bearing = "BEARING\n" + String(selected->bearingDegrees, 0) + " deg";
    const String latitude = "LATITUDE\n" + String(selected->latitude, 5);
    const String longitude = "LONGITUDE\n" + String(selected->longitude, 5);
    String normalizedEmergency = selected->emergency;
    normalizedEmergency.toLowerCase();
    const bool hasEmergency = !normalizedEmergency.isEmpty() &&
        normalizedEmergency != "none" && normalizedEmergency != "no";
    const String emergencyValue = hasEmergency ? selected->emergency : "NONE";
    const String emergency = "EMERGENCY  " + emergencyValue;
    const String values[7] = {
        altitude, speed, distance, bearing, latitude, longitude, emergency
    };
    for (uint8_t index = 0; index < 7; ++index)
        lv_label_set_text(detailMetricLabels[index], values[index].c_str());
    lv_obj_set_style_text_color(
        detailMetricLabels[6],
        Theme::color(hasEmergency ? Theme::COLOR_ERROR : Theme::COLOR_SUCCESS),
        LV_PART_MAIN);
}

void AircraftScreen::updateAircraftArtwork()
{
    if (detailArtworkImage == nullptr || selectedAircraftHex.isEmpty()) return;
    const AircraftData* selected = nullptr;
    if (aircraftService != nullptr)
        for (uint8_t index = 0; index < aircraftService->getAircraftCount(); ++index)
            if (aircraftService->getAircraft(index).hex == selectedAircraftHex)
                selected = &aircraftService->getAircraft(index);
    if (selected == nullptr) return;

    const AircraftArtwork::Family family = AircraftArtwork::classify(
        selected->type, selected->description, selected->category);
    const lv_img_dsc_t* artwork = AircraftArtwork::imageFor(family);
    if (artwork != nullptr)
    {
        if (displayedArtworkHex != selectedAircraftHex)
        {
            lv_img_set_src(detailArtworkImage, artwork);
            lv_obj_align(detailArtworkImage, LV_ALIGN_BOTTOM_MID, 0, -40);
            displayedArtworkHex = selectedAircraftHex;
        }
        lv_obj_clear_flag(detailArtworkImage, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_add_flag(detailArtworkImage, LV_OBJ_FLAG_HIDDEN);
    }
    lv_label_set_text(
        detailArtworkStatusLabel, AircraftArtwork::labelFor(family));
}

void AircraftScreen::backButtonEventHandler(lv_event_t* event)
{
    AircraftScreen* self = static_cast<AircraftScreen*>(lv_event_get_user_data(event));
    if (self != nullptr && self->navigationCallback != nullptr) self->navigationCallback(Page::Dashboard);
}

void AircraftScreen::detailBackButtonEventHandler(lv_event_t* event)
{
    AircraftScreen* self = static_cast<AircraftScreen*>(lv_event_get_user_data(event));
    if (self != nullptr) self->show();
}

void AircraftScreen::aircraftRowEventHandler(lv_event_t* event)
{
    AircraftScreen* self = static_cast<AircraftScreen*>(lv_event_get_user_data(event));
    if (self == nullptr) return;
    lv_obj_t* target = lv_event_get_target(event);
    for (uint8_t index = 0; index < AircraftService::MAX_AIRCRAFT; ++index)
        if (self->aircraftRowButtons[index] == target)
            self->showAircraftDetail(index);
}

void AircraftScreen::updateTimerCallback(lv_timer_t* timer)
{
    AircraftScreen* self = static_cast<AircraftScreen*>(timer->user_data);
    if (self != nullptr) self->update();
}
