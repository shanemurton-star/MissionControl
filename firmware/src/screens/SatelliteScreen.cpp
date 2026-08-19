#include "SatelliteScreen.h"

#include <WiFi.h>
#include <math.h>
#include <time.h>
#include "../ui/Theme.h"

namespace
{
    String localDateTime(uint32_t timestamp)
    {
        time_t value = timestamp;
        struct tm info;
        if (localtime_r(&value, &info) == nullptr) return "--";
        char buffer[24];
        strftime(buffer, sizeof(buffer), "%a %H:%M", &info);
        return buffer;
    }

    String utcDateTime(uint32_t timestamp)
    {
        time_t value = timestamp;
        struct tm info;
        if (gmtime_r(&value, &info) == nullptr) return "--";
        char buffer[28];
        strftime(buffer, sizeof(buffer), "%a %H:%M:%S UTC", &info);
        return buffer;
    }

    String compassDirection(float azimuth)
    {
        static const char* DIRECTIONS[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
        int index = static_cast<int>((azimuth + 22.5f) / 45.0f) & 7;
        return DIRECTIONS[index];
    }

    String durationText(uint32_t start, uint32_t stop)
    {
        if (stop <= start) return "--";
        uint32_t seconds = stop - start;
        return String(seconds / 60) + " min " + String(seconds % 60) + " sec";
    }

    String frequencyText(uint64_t hz)
    {
        if (hz == 0) return "--";
        const double mhz = static_cast<double>(hz) / 1000000.0;
        return String(mhz, mhz < 1000.0 ? 3 : 4) + " MHz";
    }
}

void SatelliteScreen::begin(ClockService& clockService, SatelliteService& service)
{
    if (screen != nullptr) return;
    satelliteService = &service;
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
    lv_obj_t* title = Theme::createLabel(screen, "SATELLITE PASS DETAIL", Theme::COLOR_PRIMARY);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 140, Theme::CONTENT_TOP + 10);

    lv_obj_t* sky = Theme::createPanel(screen, 8, 110, 500, 328, "LIVE SKY POSITION  |  HORIZON TO ZENITH");
    const int16_t ringSizes[] = {96, 194, 290};
    for (int16_t size : ringSizes)
    {
        lv_obj_t* ring = lv_obj_create(sky);
        lv_obj_set_size(ring, size, size);
        lv_obj_align(ring, LV_ALIGN_CENTER, 0, 10);
        lv_obj_set_style_radius(ring, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(ring, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(ring, 1, LV_PART_MAIN);
        lv_obj_set_style_border_color(ring, Theme::color(Theme::COLOR_PANEL_BORDER), LV_PART_MAIN);
        lv_obj_clear_flag(ring, LV_OBJ_FLAG_SCROLLABLE);
    }
    lv_obj_t* zenith = Theme::createLabel(sky, "+ ZENITH", Theme::COLOR_TEXT_DIM);
    lv_obj_align(zenith, LV_ALIGN_CENTER, 0, 10);
    lv_obj_t* north = Theme::createLabel(sky, "N", Theme::COLOR_TEXT_DIM);
    lv_obj_align(north, LV_ALIGN_TOP_MID, 0, 24);
    lv_obj_t* east = Theme::createLabel(sky, "E", Theme::COLOR_TEXT_DIM);
    lv_obj_align(east, LV_ALIGN_RIGHT_MID, -72, 10);
    lv_obj_t* south = Theme::createLabel(sky, "S", Theme::COLOR_TEXT_DIM);
    lv_obj_align(south, LV_ALIGN_BOTTOM_MID, 0, -24);
    lv_obj_t* west = Theme::createLabel(sky, "W", Theme::COLOR_TEXT_DIM);
    lv_obj_align(west, LV_ALIGN_LEFT_MID, 72, 10);

    for (uint8_t i = 0; i < SatelliteService::SATELLITE_COUNT; ++i)
    {
        targets[i] = lv_obj_create(sky);
        lv_obj_set_size(targets[i], 9, 9);
        lv_obj_set_style_radius(targets[i], LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_bg_color(targets[i], Theme::color(Theme::COLOR_SUCCESS), LV_PART_MAIN);
        lv_obj_set_style_border_width(targets[i], 0, LV_PART_MAIN);
        lv_obj_clear_flag(targets[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(targets[i], LV_OBJ_FLAG_HIDDEN);
        targetLabels[i] = Theme::createLabel(sky, "", Theme::COLOR_SUCCESS);
        lv_obj_add_flag(targetLabels[i], LV_OBJ_FLAG_HIDDEN);
    }

    lv_obj_t* passes = Theme::createPanel(screen, 516, 110, 276, 328, "UPCOMING PASSES  |  TOUCH FOR DETAIL");
    passScroller = lv_obj_create(passes);
    lv_obj_set_pos(passScroller, 0, 28);
    lv_obj_set_size(passScroller, 252, 288);
    lv_obj_set_style_bg_opa(passScroller, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(passScroller, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(passScroller, 0, LV_PART_MAIN);
    lv_obj_set_scroll_dir(passScroller, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(passScroller, LV_SCROLLBAR_MODE_AUTO);

    statusLabel = Theme::createLabel(passScroller, "WAITING FOR ORBIT DATA", Theme::COLOR_WARNING);
    lv_obj_align(statusLabel, LV_ALIGN_TOP_LEFT, 0, 2);
    for (uint8_t row = 0; row < SatelliteService::SATELLITE_COUNT; ++row)
    {
        passRowButtons[row] = lv_btn_create(passScroller);
        lv_obj_set_pos(passRowButtons[row], 0, row * 76);
        lv_obj_set_size(passRowButtons[row], 242, 70);
        lv_obj_set_style_bg_color(passRowButtons[row], Theme::color(Theme::COLOR_PANEL), LV_PART_MAIN);
        lv_obj_set_style_border_color(passRowButtons[row], Theme::color(Theme::COLOR_PANEL_BORDER), LV_PART_MAIN);
        lv_obj_set_style_border_width(passRowButtons[row], 1, LV_PART_MAIN);
        lv_obj_set_style_radius(passRowButtons[row], 4, LV_PART_MAIN);
        lv_obj_set_style_pad_all(passRowButtons[row], 6, LV_PART_MAIN);
        lv_obj_add_event_cb(passRowButtons[row], satelliteRowEventHandler, LV_EVENT_CLICKED, this);
        passRowLabels[row] = Theme::createLabel(
            passRowButtons[row], "", Theme::COLOR_TEXT_MUTED, &lv_font_montserrat_14);
        lv_obj_set_width(passRowLabels[row], 228);
        lv_obj_align(passRowLabels[row], LV_ALIGN_LEFT_MID, 0, 0);
        lv_obj_add_flag(passRowButtons[row], LV_OBJ_FLAG_HIDDEN);
    }

    detailScreen = lv_obj_create(nullptr);
    Theme::configureScreen(detailScreen);
    detailHeaderBar.create(detailScreen, clockService, Theme::SCREEN_WIDTH, Theme::HEADER_HEIGHT);
    detailHeaderBar.setSettingsCallback([this]() {
        if (navigationCallback != nullptr) navigationCallback(Page::Settings);
    });
    lv_obj_t* detailBackButton = lv_btn_create(detailScreen);
    lv_obj_set_pos(detailBackButton, 8, Theme::CONTENT_TOP);
    lv_obj_set_size(detailBackButton, 140, 36);
    lv_obj_set_style_bg_color(detailBackButton, Theme::color(Theme::COLOR_PANEL), LV_PART_MAIN);
    lv_obj_set_style_border_color(detailBackButton, Theme::color(Theme::COLOR_PANEL_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(detailBackButton, 1, LV_PART_MAIN);
    lv_obj_add_event_cb(detailBackButton, detailBackButtonEventHandler, LV_EVENT_CLICKED, this);
    lv_obj_center(Theme::createLabel(detailBackButton, LV_SYMBOL_LEFT " SATELLITES", Theme::COLOR_PRIMARY));
    detailTitleLabel = Theme::createLabel(detailScreen, "SATELLITE INFORMATION", Theme::COLOR_PRIMARY);
    lv_obj_align(detailTitleLabel, LV_ALIGN_TOP_LEFT, 164, Theme::CONTENT_TOP + 10);

    lv_obj_t* identityPanel = Theme::createPanel(detailScreen, 8, 110, 252, 328, "IDENTITY / RADIO  |  SATNOGS");
    lv_obj_add_flag(identityPanel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(identityPanel, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(identityPanel, LV_SCROLLBAR_MODE_AUTO);
    detailIdentityLabel = Theme::createLabel(identityPanel, "", Theme::COLOR_TEXT, &lv_font_montserrat_14);
    lv_obj_set_pos(detailIdentityLabel, 0, 38);
    lv_obj_set_width(detailIdentityLabel, 222);
    lv_obj_t* positionPanel = Theme::createPanel(detailScreen, 268, 110, 252, 328, "LIVE POSITION");
    detailPositionLabel = Theme::createLabel(positionPanel, "", Theme::COLOR_TEXT_MUTED, &lv_font_montserrat_14);
    lv_obj_set_pos(detailPositionLabel, 0, 38);
    lv_obj_set_width(detailPositionLabel, 222);
    lv_obj_t* passPanel = Theme::createPanel(detailScreen, 528, 110, 264, 328, "NEXT PASS");
    lv_obj_add_flag(passPanel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(passPanel, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(passPanel, LV_SCROLLBAR_MODE_AUTO);
    detailPassLabel = Theme::createLabel(passPanel, "", Theme::COLOR_TEXT_MUTED, &lv_font_montserrat_14);
    lv_obj_set_pos(detailPassLabel, 0, 38);
    lv_obj_set_width(detailPassLabel, 234);
    update();
    updateTimer = lv_timer_create(updateTimerCallback, 1000, this);
}

void SatelliteScreen::show() { if (screen != nullptr) { lv_scr_load(screen); update(); } }
void SatelliteScreen::setNavigationCallback(NavigationCallback callback) { navigationCallback = callback; }

void SatelliteScreen::update()
{
    headerBar.update();
    if (satelliteService == nullptr) return;
    if (!satelliteService->isValid())
    {
        lv_obj_clear_flag(statusLabel, LV_OBJ_FLAG_HIDDEN);
        if (WiFi.status() != WL_CONNECTED) lv_label_set_text(statusLabel, "WAITING FOR WIFI");
        else if (satelliteService->isUpdating()) lv_label_set_text(statusLabel, "UPDATING ORBIT DATA...");
        else lv_label_set_text(statusLabel, satelliteService->getLastError().c_str());
        return;
    }
    lv_obj_add_flag(statusLabel, LV_OBJ_FLAG_HIDDEN);

    uint8_t order[SatelliteService::SATELLITE_COUNT] = {0, 1, 2, 3, 4};
    for (uint8_t i = 1; i < SatelliteService::SATELLITE_COUNT; ++i)
    {
        uint8_t value = order[i];
        int8_t position = i - 1;
        while (position >= 0 && satelliteService->getSatellite(order[position]).aosTime >
               satelliteService->getSatellite(value).aosTime)
        {
            order[position + 1] = order[position];
            --position;
        }
        order[position + 1] = value;
    }

    for (uint8_t row = 0; row < SatelliteService::SATELLITE_COUNT; ++row)
    {
        const uint8_t satelliteIndex = order[row];
        const SatelliteData& item = satelliteService->getSatellite(satelliteIndex);
        passRowSatelliteIndices[row] = satelliteIndex;
        String text = item.name + "\n" + localDateTime(item.aosTime) +
            "   MAX " + String(item.maxElevation, 0) + "deg\n";
        text += "AOS " + compassDirection(item.aosAzimuth) + "   LOS " +
            compassDirection(item.losAzimuth) + "   " LV_SYMBOL_RIGHT;
        lv_label_set_text(passRowLabels[row], text.c_str());
        lv_obj_clear_flag(passRowButtons[row], LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_update_layout(passScroller);

    for (uint8_t i = 0; i < SatelliteService::SATELLITE_COUNT; ++i)
    {
        const SatelliteData& item = satelliteService->getSatellite(i);
        if (!item.visible)
        {
            lv_obj_add_flag(targets[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(targetLabels[i], LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        const float radius = (90.0f - item.currentElevation) / 90.0f * 145.0f;
        const float radians = item.currentAzimuth * DEG_TO_RAD;
        const int16_t x = static_cast<int16_t>(sin(radians) * radius);
        const int16_t y = static_cast<int16_t>(-cos(radians) * radius) + 10;
        lv_obj_align(targets[i], LV_ALIGN_CENTER, x, y);
        lv_obj_clear_flag(targets[i], LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(targetLabels[i], item.name.c_str());
        lv_obj_align(targetLabels[i], LV_ALIGN_CENTER, x + 35, y - 10);
        lv_obj_clear_flag(targetLabels[i], LV_OBJ_FLAG_HIDDEN);
    }

    if (lv_scr_act() == detailScreen) updateDetail();
}

void SatelliteScreen::showSatelliteDetail(uint8_t satelliteIndex)
{
    if (satelliteService == nullptr || satelliteIndex >= SatelliteService::SATELLITE_COUNT) return;
    selectedCatalogNumber = satelliteService->getSatellite(satelliteIndex).catalogNumber;
    satelliteService->requestRadioData(satelliteIndex);
    updateDetail();
    lv_scr_load(detailScreen);
}

void SatelliteScreen::updateDetail()
{
    detailHeaderBar.update();
    if (satelliteService == nullptr || selectedCatalogNumber == 0) return;
    const SatelliteData* selected = nullptr;
    for (uint8_t index = 0; index < SatelliteService::SATELLITE_COUNT; ++index)
        if (satelliteService->getSatellite(index).catalogNumber == selectedCatalogNumber)
            selected = &satelliteService->getSatellite(index);
    if (selected == nullptr) return;

    lv_label_set_text(detailTitleLabel, selected->name.c_str());
    String identity = "NAME\n" + selected->name +
        "\n\nNORAD CATALOG\n" + String(selected->catalogNumber) +
        "\n\nORBIT DATA\n" + (selected->valid ? String("CURRENT") : String("UNAVAILABLE")) +
        "\n\nTRACKING\n" + (selected->visible ? String("ABOVE HORIZON") : String("BELOW HORIZON"));

    identity += "\n\nRADIO FREQUENCIES\n";
    if (!selected->radioReady)
        identity += "Loading from SatNOGS...";
    else if (!selected->radioError.isEmpty())
        identity += selected->radioError;
    else if (selected->radioChannelCount == 0)
        identity += "No active transmitters listed";
    else
    {
        for (uint8_t index = 0; index < selected->radioChannelCount; ++index)
        {
            const SatelliteRadioChannel& channel = selected->radioChannels[index];
            if (index > 0) identity += "\n\n";
            identity += channel.description + "\n";
            if (channel.downlinkHz > 0)
                identity += "DOWN  " + frequencyText(channel.downlinkHz) + "\n";
            if (channel.uplinkHz > 0)
                identity += "UP       " + frequencyText(channel.uplinkHz) + "\n";
            identity += "MODE  " + channel.mode;
            if (channel.baud > 0) identity += "  " + String(channel.baud) + " baud";
            if (!channel.service.isEmpty()) identity += "\n" + channel.service;
        }
    }
    identity += "\n\nSOURCE\nSatNOGS DB";
    lv_label_set_text(detailIdentityLabel, identity.c_str());

    String position = "STATUS\n" + (selected->visible ? String("VISIBLE / IN VIEW") : String("NOT IN VIEW")) +
        "\n\nAZIMUTH\n" + String(selected->currentAzimuth, 1) + " deg  " +
        compassDirection(selected->currentAzimuth) +
        "\n\nELEVATION\n" + String(selected->currentElevation, 1) + " deg" +
        "\n\nSLANT RANGE\n" + String(selected->rangeKm, 0) + " km";
    lv_label_set_text(detailPositionLabel, position.c_str());

    String pass = "AOS - LOCAL\n" + localDateTime(selected->aosTime) +
        "\n\nAOS - UTC\n" + utcDateTime(selected->aosTime) +
        "\n\nPEAK - LOCAL\n" + localDateTime(selected->maxTime) +
        "\n\nLOS - LOCAL\n" + localDateTime(selected->losTime) +
        "\n\nDURATION\n" + durationText(selected->aosTime, selected->losTime) +
        "\n\nMAX ELEVATION\n" + String(selected->maxElevation, 1) + " deg" +
        "\n\nRISE DIRECTION\n" + String(selected->aosAzimuth, 1) + " deg  " +
        compassDirection(selected->aosAzimuth) +
        "\n\nSET DIRECTION\n" + String(selected->losAzimuth, 1) + " deg  " +
        compassDirection(selected->losAzimuth);
    lv_label_set_text(detailPassLabel, pass.c_str());
}

void SatelliteScreen::backButtonEventHandler(lv_event_t* event)
{
    SatelliteScreen* self = static_cast<SatelliteScreen*>(lv_event_get_user_data(event));
    if (self != nullptr && self->navigationCallback != nullptr) self->navigationCallback(Page::Dashboard);
}

void SatelliteScreen::detailBackButtonEventHandler(lv_event_t* event)
{
    SatelliteScreen* self = static_cast<SatelliteScreen*>(lv_event_get_user_data(event));
    if (self != nullptr) self->show();
}

void SatelliteScreen::satelliteRowEventHandler(lv_event_t* event)
{
    SatelliteScreen* self = static_cast<SatelliteScreen*>(lv_event_get_user_data(event));
    if (self == nullptr) return;
    lv_obj_t* target = lv_event_get_target(event);
    for (uint8_t row = 0; row < SatelliteService::SATELLITE_COUNT; ++row)
        if (self->passRowButtons[row] == target)
            self->showSatelliteDetail(self->passRowSatelliteIndices[row]);
}
void SatelliteScreen::updateTimerCallback(lv_timer_t* timer)
{
    SatelliteScreen* self = static_cast<SatelliteScreen*>(timer->user_data);
    if (self != nullptr) self->update();
}
