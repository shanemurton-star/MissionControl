#include "AircraftScreen.h"

#include <WiFi.h>
#include <math.h>
#include <time.h>
#include "../ui/Theme.h"

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

    lv_obj_t* backButton = lv_btn_create(screen);
    lv_obj_set_pos(backButton, 8, Theme::CONTENT_TOP);
    lv_obj_set_size(backButton, 116, 36);
    lv_obj_set_style_bg_color(backButton, Theme::color(Theme::COLOR_PANEL), LV_PART_MAIN);
    lv_obj_set_style_border_color(backButton, Theme::color(Theme::COLOR_PANEL_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(backButton, 1, LV_PART_MAIN);
    lv_obj_add_event_cb(backButton, backButtonEventHandler, LV_EVENT_CLICKED, this);
    lv_obj_center(Theme::createLabel(backButton, LV_SYMBOL_LEFT " DASHBOARD", Theme::COLOR_PRIMARY));

    lv_obj_t* title = Theme::createLabel(screen, "LIVE AIRCRAFT DETAIL", Theme::COLOR_PRIMARY);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 140, Theme::CONTENT_TOP + 10);
    countLabel = Theme::createLabel(screen, "-- AIRCRAFT WITHIN 25 NM", Theme::COLOR_TEXT);
    lv_obj_align(countLabel, LV_ALIGN_TOP_RIGHT, -8, Theme::CONTENT_TOP + 10);

    lv_obj_t* scope = Theme::createPanel(screen, 8, 110, 500, 328, "AIRSPACE  |  25 NM RANGE");
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

    for (uint8_t i = 0; i < AircraftService::MAX_AIRCRAFT; ++i)
    {
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

    lv_obj_t* listPanel = Theme::createPanel(screen, 516, 110, 276, 328, "NEAREST AIRCRAFT  |  ADSB.FI");
    statusLabel = Theme::createLabel(listPanel, "WAITING FOR LIVE ADS-B", Theme::COLOR_WARNING);
    lv_obj_align(statusLabel, LV_ALIGN_TOP_LEFT, 0, 30);
    for (uint8_t index = 0; index < 6; ++index)
    {
        aircraftRowButtons[index] = lv_btn_create(listPanel);
        lv_obj_set_pos(aircraftRowButtons[index], 0, 28 + index * 46);
        lv_obj_set_size(aircraftRowButtons[index], 254, 42);
        lv_obj_set_style_bg_color(
            aircraftRowButtons[index], Theme::color(Theme::COLOR_PANEL), LV_PART_MAIN);
        lv_obj_set_style_border_color(
            aircraftRowButtons[index], Theme::color(Theme::COLOR_PANEL_BORDER), LV_PART_MAIN);
        lv_obj_set_style_border_width(aircraftRowButtons[index], 1, LV_PART_MAIN);
        lv_obj_set_style_radius(aircraftRowButtons[index], 4, LV_PART_MAIN);
        lv_obj_set_style_pad_all(aircraftRowButtons[index], 4, LV_PART_MAIN);
        lv_obj_add_event_cb(
            aircraftRowButtons[index], aircraftRowEventHandler, LV_EVENT_CLICKED, this);
        aircraftRowLabels[index] = Theme::createLabel(
            aircraftRowButtons[index], "", Theme::COLOR_TEXT_MUTED);
        lv_obj_set_width(aircraftRowLabels[index], 242);
        lv_obj_align(aircraftRowLabels[index], LV_ALIGN_LEFT_MID, 0, 0);
        lv_obj_add_flag(aircraftRowButtons[index], LV_OBJ_FLAG_HIDDEN);
    }

    detailScreen = lv_obj_create(nullptr);
    Theme::configureScreen(detailScreen);
    detailHeaderBar.create(detailScreen, clockService, Theme::SCREEN_WIDTH, Theme::HEADER_HEIGHT);
    detailHeaderBar.setSettingsCallback([this]() {
        if (navigationCallback != nullptr) navigationCallback(Page::Settings);
    });
    lv_obj_t* detailBackButton = lv_btn_create(detailScreen);
    lv_obj_set_pos(detailBackButton, 8, Theme::CONTENT_TOP);
    lv_obj_set_size(detailBackButton, 132, 36);
    lv_obj_set_style_bg_color(detailBackButton, Theme::color(Theme::COLOR_PANEL), LV_PART_MAIN);
    lv_obj_set_style_border_color(detailBackButton, Theme::color(Theme::COLOR_PANEL_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(detailBackButton, 1, LV_PART_MAIN);
    lv_obj_add_event_cb(detailBackButton, detailBackButtonEventHandler, LV_EVENT_CLICKED, this);
    lv_obj_center(Theme::createLabel(detailBackButton, LV_SYMBOL_LEFT " AIRCRAFT", Theme::COLOR_PRIMARY));
    lv_obj_t* detailTitle = Theme::createLabel(detailScreen, "AIRCRAFT INFORMATION", Theme::COLOR_PRIMARY);
    lv_obj_align(detailTitle, LV_ALIGN_TOP_LEFT, 156, Theme::CONTENT_TOP + 10);

    lv_obj_t* identityPanel = Theme::createPanel(detailScreen, 8, 110, 252, 328, "IDENTITY");
    lv_obj_add_flag(identityPanel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(identityPanel, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(identityPanel, LV_SCROLLBAR_MODE_AUTO);
    detailIdentityLabel = Theme::createLabel(identityPanel, "", Theme::COLOR_TEXT, &lv_font_montserrat_14);
    lv_obj_set_pos(detailIdentityLabel, 0, 38);
    lv_obj_set_width(detailIdentityLabel, 222);
    lv_obj_t* flightPanel = Theme::createPanel(detailScreen, 268, 110, 252, 328, "FLIGHT DATA");
    lv_obj_add_flag(flightPanel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(flightPanel, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(flightPanel, LV_SCROLLBAR_MODE_AUTO);
    detailFlightLabel = Theme::createLabel(flightPanel, "", Theme::COLOR_TEXT_MUTED, &lv_font_montserrat_14);
    lv_obj_set_pos(detailFlightLabel, 0, 38);
    lv_obj_set_width(detailFlightLabel, 222);
    lv_obj_t* positionPanel = Theme::createPanel(detailScreen, 528, 110, 264, 328, "POSITION");
    lv_obj_add_flag(positionPanel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(positionPanel, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(positionPanel, LV_SCROLLBAR_MODE_AUTO);
    detailPositionLabel = Theme::createLabel(positionPanel, "", Theme::COLOR_TEXT_MUTED, &lv_font_montserrat_14);
    lv_obj_set_pos(detailPositionLabel, 0, 38);
    lv_obj_set_width(detailPositionLabel, 234);

    update();
    updateTimer = lv_timer_create(updateTimerCallback, 1000, this);
}

void AircraftScreen::show() { if (screen != nullptr) { lv_scr_load(screen); update(); } }
void AircraftScreen::setNavigationCallback(NavigationCallback callback) { navigationCallback = callback; }

void AircraftScreen::update()
{
    headerBar.update();
    if (aircraftService == nullptr) return;
    const uint8_t count = aircraftService->getAircraftCount();
    String countText = String(count) + " AIRCRAFT WITHIN 25 NM";
    lv_label_set_text(countLabel, countText.c_str());

    if (!aircraftService->isValid())
    {
        lv_obj_clear_flag(statusLabel, LV_OBJ_FLAG_HIDDEN);
        if (WiFi.status() != WL_CONNECTED) lv_label_set_text(statusLabel, "WAITING FOR WIFI");
        else if (aircraftService->isUpdating()) lv_label_set_text(statusLabel, "UPDATING LIVE TRAFFIC...");
        else lv_label_set_text(statusLabel, aircraftService->getLastError().c_str());
        return;
    }
    lv_obj_add_flag(statusLabel, LV_OBJ_FLAG_HIDDEN);

    const uint8_t rows = count < 6 ? count : 6;
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
    }
    for (uint8_t i = rows; i < 6; ++i)
        lv_obj_add_flag(aircraftRowButtons[i], LV_OBJ_FLAG_HIDDEN);
    if (count == 0)
    {
        lv_obj_clear_flag(statusLabel, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(statusLabel, "NO POSITIONED AIRCRAFT\nCURRENTLY WITHIN RANGE");
    }

    for (uint8_t i = 0; i < AircraftService::MAX_AIRCRAFT; ++i)
    {
        if (i >= count)
        {
            lv_obj_add_flag(targets[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(targetLabels[i], LV_OBJ_FLAG_HIDDEN);
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
        lv_obj_align(targetLabels[i], LV_ALIGN_CENTER, x + 28, y - 9);
        lv_obj_clear_flag(targetLabels[i], LV_OBJ_FLAG_HIDDEN);
    }

    if (lv_scr_act() == detailScreen) updateDetail();
}

void AircraftScreen::showAircraftDetail(uint8_t index)
{
    if (aircraftService == nullptr || index >= aircraftService->getAircraftCount()) return;
    selectedAircraftHex = aircraftService->getAircraft(index).hex;
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
        lv_label_set_text(detailIdentityLabel, "Aircraft no longer\nwithin range");
        return;
    }

    String identity = "CALLSIGN\n" + (selected->callsign.isEmpty() ? String("--") : selected->callsign) +
        "\n\nREGISTRATION\n" + (selected->registration.isEmpty() ? String("--") : selected->registration) +
        "\n\nICAO HEX\n" + selected->hex +
        "\n\nTYPE CODE\n" + (selected->type.isEmpty() ? String("--") : selected->type) +
        "\n\nAIRCRAFT\n" + (selected->description.isEmpty() ? String("--") : selected->description) +
        "\n\nCATEGORY\n" + (selected->category.isEmpty() ? String("--") : selected->category);
    lv_label_set_text(detailIdentityLabel, identity.c_str());

    String flight = "ALTITUDE\n" + (selected->onGround ? String("GROUND") : String(selected->altitudeFeet, 0) + " ft") +
        "\n\nGROUND SPEED\n" + String(selected->groundSpeedKnots, 0) + " kt" +
        "\n\nINDICATED SPEED\n" + String(selected->indicatedSpeedKnots, 0) + " kt" +
        "\n\nMACH\n" + String(selected->mach, 3) +
        "\n\nTRACK\n" + String(selected->trackDegrees, 0) + " deg" +
        "\n\nVERTICAL RATE\n" + String(selected->verticalRateFpm) + " ft/min" +
        "\n\nSELECTED ALTITUDE\n" + String(selected->selectedAltitudeFeet, 0) + " ft" +
        "\n\nSELECTED HEADING\n" + String(selected->selectedHeadingDegrees, 0) + " deg" +
        "\n\nSQUAWK\n" + (selected->squawk.isEmpty() ? String("--") : selected->squawk);
    lv_label_set_text(detailFlightLabel, flight.c_str());

    String position = "DISTANCE\n" + String(selected->distanceNm, 1) + " nm" +
        "\n\nBEARING\n" + String(selected->bearingDegrees, 0) + " deg" +
        "\n\nLATITUDE\n" + String(selected->latitude, 5) +
        "\n\nLONGITUDE\n" + String(selected->longitude, 5) +
        "\n\nLAST POSITION\n" + String(selected->seenSeconds, 1) + " sec ago";
    if (!selected->emergency.isEmpty())
        position += "\n\nEMERGENCY\n" + selected->emergency;
    lv_label_set_text(detailPositionLabel, position.c_str());
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
    for (uint8_t index = 0; index < 6; ++index)
        if (self->aircraftRowButtons[index] == target)
            self->showAircraftDetail(index);
}

void AircraftScreen::updateTimerCallback(lv_timer_t* timer)
{
    AircraftScreen* self = static_cast<AircraftScreen*>(timer->user_data);
    if (self != nullptr) self->update();
}
