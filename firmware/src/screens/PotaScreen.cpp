#include "PotaScreen.h"
#include "../services/NetworkUpdateState.h"

#include <WiFi.h>

#include "../ui/Theme.h"

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
    lv_obj_set_size(backButton, 116, 36);
    lv_obj_set_style_bg_color(backButton, Theme::color(Theme::COLOR_PANEL), LV_PART_MAIN);
    lv_obj_set_style_border_color(backButton, Theme::color(Theme::COLOR_PANEL_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(backButton, 1, LV_PART_MAIN);
    lv_obj_add_event_cb(backButton, backButtonEventHandler, LV_EVENT_CLICKED, this);
    lv_obj_center(Theme::createLabel(backButton, LV_SYMBOL_LEFT " DASHBOARD", Theme::COLOR_PRIMARY));

    lv_obj_t* title = Theme::createLabel(screen, "POTA SPOTS DETAIL", Theme::COLOR_PRIMARY);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 140, Theme::CONTENT_TOP + 10);
    statusLabel = Theme::createLabel(screen, "", Theme::COLOR_TEXT_MUTED);
    lv_obj_align(statusLabel, LV_ALIGN_TOP_RIGHT, -10, Theme::CONTENT_TOP + 10);

    lv_obj_t* left = Theme::createPanel(screen, 8, 110, 500, 328, "ACTIVE PARKS WITHIN 100 MILES");
    lv_obj_add_flag(left, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(left, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(left, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_t* right = Theme::createPanel(screen, 516, 110, 276, 328, "POTA SUMMARY");
    columnLabels[0] = Theme::createLabel(left, "WAITING FOR POTA", Theme::COLOR_TEXT_MUTED);
    columnLabels[1] = Theme::createLabel(right, "", Theme::COLOR_TEXT_MUTED);
    lv_obj_set_pos(columnLabels[0], 0, 32);
    lv_obj_set_pos(columnLabels[1], 0, 32);
    lv_obj_set_width(columnLabels[0], 466);
    lv_obj_set_width(columnLabels[1], 246);

    update();
    updateTimer = lv_timer_create(updateTimerCallback, 1000, this);
}

void PotaScreen::show()
{
    if (screen != nullptr) { lv_scr_load(screen); update(); }
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
    if (!potaService->isValid())
    {
        const char* status = WiFi.status() != WL_CONNECTED ? "WAITING FOR WIFI" :
            (potaService->isUpdating() ? "UPDATING POTA SPOTS..." : potaService->getLastError().c_str());
        lv_label_set_text(columnLabels[0], status);
        return;
    }

    const uint8_t count = potaService->getSpotCount();
    lv_label_set_text(statusLabel, (String(count) + " ACTIVE PARKS  |  60 SEC REFRESH").c_str());
    String list;
    for (uint8_t index = 0; index < potaService->getSpotCount(); ++index)
    {
        const PotaSpotData& spot = potaService->getSpot(index);
        String parkName = spot.name;
        if (parkName.length() > 42) parkName = parkName.substring(0, 42);
        if (!list.isEmpty()) list += "\n\n";
        list += spot.reference + "   " + String(spot.distanceMiles, 0) + " mi\n";
        list += parkName + "\nActivator: " + spot.activator;
    }
    if (list.isEmpty()) list = "NO ACTIVE PARKS WITHIN 100 MILES";
    lv_label_set_text(columnLabels[0], list.c_str());
    String summary = "ACTIVE PARKS\n" + String(count) +
        "\n\nSEARCH RADIUS\n100 miles" +
        "\n\nSORT ORDER\nNearest first" +
        "\n\nREFRESH\nEvery 60 seconds" +
        "\n\nTap and drag the list\nto view more parks.";
    lv_label_set_text(columnLabels[1], summary.c_str());
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
