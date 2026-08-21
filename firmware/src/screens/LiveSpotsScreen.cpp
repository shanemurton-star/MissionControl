#include "LiveSpotsScreen.h"
#include "../services/NetworkUpdateState.h"
#include <WiFi.h>
#include "../ui/Theme.h"

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
    lv_obj_set_size(backButton, 116, 36);
    lv_obj_set_style_bg_color(backButton, Theme::color(Theme::COLOR_PANEL), LV_PART_MAIN);
    lv_obj_set_style_border_color(backButton, Theme::color(Theme::COLOR_PANEL_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(backButton, 1, LV_PART_MAIN);
    lv_obj_add_event_cb(backButton, backButtonEventHandler, LV_EVENT_CLICKED, this);
    lv_obj_center(Theme::createLabel(backButton, LV_SYMBOL_LEFT " DASHBOARD", Theme::COLOR_PRIMARY));
    titleLabel = Theme::createLabel(screen, "LIVE PSK SPOTS", Theme::COLOR_PRIMARY);
    lv_obj_align(titleLabel, LV_ALIGN_TOP_LEFT, 140, Theme::CONTENT_TOP + 10);

    lv_obj_t* bandPanel = Theme::createPanel(screen, 8, 110, 380, 328, "BAND ACTIVITY  |  LAST HOUR");
    summaryLabel = Theme::createLabel(bandPanel, "WAITING FOR DATA", Theme::COLOR_TEXT_MUTED);
    lv_obj_align(summaryLabel, LV_ALIGN_TOP_LEFT, 0, 34);
    lv_obj_t* recentPanel = Theme::createPanel(screen, 396, 110, 396, 328, "MOST RECENT REPORTS");
    statusLabel = Theme::createLabel(recentPanel, "WAITING FOR PSK REPORTER", Theme::COLOR_WARNING);
    lv_obj_align(statusLabel, LV_ALIGN_TOP_LEFT, 0, 34);
    recentLabel = Theme::createLabel(recentPanel, "", Theme::COLOR_TEXT_MUTED);
    lv_obj_align(recentLabel, LV_ALIGN_TOP_LEFT, 0, 34);
    update();
    updateTimer = lv_timer_create(updateTimerCallback, 1000, this);
}

void LiveSpotsScreen::show() { if (screen != nullptr) { lv_scr_load(screen); update(); } }
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
    String summary = "BAND   SPOTS   FARTHEST\n\n";
    for (uint8_t i = 0; i < LiveSpotsService::BAND_COUNT; ++i)
    {
        const BandSpotSummary& band = service->getBand(i);
        summary += String(band.name) + "      " + String(band.count) + "      ";
        if (band.count > 0)
            summary += String(band.farthestKm, 0) + " km  " + band.farthestCall;
        else summary += "--";
        if (i + 1 < LiveSpotsService::BAND_COUNT) summary += "\n\n";
    }
    lv_label_set_text(summaryLabel, summary.c_str());

    String recent;
    const uint8_t count = service->getRecentSpotCount() < 9 ? service->getRecentSpotCount() : 9;
    for (uint8_t i = 0; i < count; ++i)
    {
        const LiveSpot& spot = service->getRecentSpot(i);
        recent += String(spot.band) + "  " + spot.mode + "  " + spot.senderCallsign;
        if (!spot.senderLocator.isEmpty()) recent += "  " + spot.senderLocator;
        recent += "\n" + String(spot.frequencyMhz, 3) + " MHz  " +
            String(spot.snr) + " dB  " + String(spot.distanceKm, 0) + " km";
        if (i + 1 < count) recent += "\n\n";
    }
    if (count == 0) recent = "NO REPORTS FROM " + service->getGridSquare() + "\nIN THE LAST HOUR";
    lv_label_set_text(recentLabel, recent.c_str());
}

void LiveSpotsScreen::backButtonEventHandler(lv_event_t* event)
{
    LiveSpotsScreen* self = static_cast<LiveSpotsScreen*>(lv_event_get_user_data(event));
    if (self != nullptr && self->navigationCallback != nullptr) self->navigationCallback(Page::Dashboard);
}
void LiveSpotsScreen::updateTimerCallback(lv_timer_t* timer)
{
    LiveSpotsScreen* self = static_cast<LiveSpotsScreen*>(timer->user_data);
    if (self != nullptr) self->update();
}
