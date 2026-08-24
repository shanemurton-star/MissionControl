#include "HeaderBar.h"

#include <WiFi.h>
#include "Theme.h"

const AppSettings* HeaderBar::appSettings = nullptr;
HeaderBar::ScreenOffCallback HeaderBar::screenOffCallback = nullptr;

namespace
{
    constexpr Page MENU_PAGES[] = {
        Page::Dashboard,
        Page::Weather,
        Page::Aircraft,
        Page::LiveSpots,
        Page::Solar,
        Page::Satellite,
        Page::Pota,
        Page::Settings
    };

    constexpr const char* MENU_LABELS[] = {
        "DASHBOARD",
        "WEATHER",
        "AIRCRAFT",
        "LIVE SPOTS",
        "SOLAR CONDITIONS",
        "SATELLITES",
        "POTA SPOTS",
        "SETTINGS"
    };

    lv_obj_t* createLine(lv_obj_t* parent, int16_t x, int16_t y, int16_t width)
    {
        lv_obj_t* line = lv_obj_create(parent);
        lv_obj_set_pos(line, x, y);
        lv_obj_set_size(line, width, 3);
        lv_obj_set_style_bg_color(line, Theme::color(Theme::COLOR_TEXT), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(line, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(line, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(line, 2, LV_PART_MAIN);
        lv_obj_set_style_pad_all(line, 0, LV_PART_MAIN);
        lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE);
        return line;
    }

    void configureMenuRow(lv_obj_t* row)
    {
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_bg_color(
            row, Theme::color(Theme::COLOR_PRIMARY),
            LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(
            row, LV_OPA_20, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
        lv_obj_set_style_outline_width(row, 0, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(row, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(row, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(row, 6, LV_PART_MAIN);
    }

    void createMenuSeparator(
        lv_obj_t* parent, int16_t y, lv_opa_t opacity = LV_OPA_50)
    {
        lv_obj_t* separator = lv_obj_create(parent);
        lv_obj_set_pos(separator, 0, y);
        lv_obj_set_size(separator, 248, 1);
        lv_obj_set_style_bg_color(
            separator, Theme::color(Theme::COLOR_PANEL_BORDER), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(separator, opacity, LV_PART_MAIN);
        lv_obj_set_style_border_width(separator, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(separator, 0, LV_PART_MAIN);
        lv_obj_clear_flag(separator, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(separator, LV_OBJ_FLAG_CLICKABLE);
    }
}

void HeaderBar::configureSettings(const AppSettings& settings)
{
    appSettings = &settings;
}

void HeaderBar::configureScreenOffCallback(ScreenOffCallback callback)
{
    screenOffCallback = callback;
}

void HeaderBar::create(lv_obj_t* parent, ClockService& clockServiceReference,
                       int16_t width, int16_t height)
{
    clockService = &clockServiceReference;

    container = lv_obj_create(parent);
    lv_obj_set_pos(container, 0, 0);
    lv_obj_set_size(container, width, height);
    Theme::configureHeader(container);

    lv_obj_t* menuTarget = lv_obj_create(container);
    lv_obj_set_pos(menuTarget, 8, 5);
    lv_obj_set_size(menuTarget, 62, height - 10);
    lv_obj_set_style_bg_opa(menuTarget, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(menuTarget, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(menuTarget, 0, LV_PART_MAIN);
    lv_obj_add_flag(menuTarget, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(menuTarget, LV_OBJ_FLAG_SCROLLABLE);
    createLine(menuTarget, 15, 14, 30);
    createLine(menuTarget, 15, 24, 30);
    createLine(menuTarget, 15, 34, 30);
    lv_obj_add_event_cb(menuTarget, menuEventHandler, LV_EVENT_CLICKED, this);

    identityLabel = Theme::createLabel(container, "-- - ----", Theme::COLOR_TEXT,
                                       &lv_font_montserrat_28);
    lv_obj_align(identityLabel, LV_ALIGN_CENTER, 0, 0);

    wifiStatusLabel = Theme::createLabel(container, LV_SYMBOL_WIFI,
                                         Theme::COLOR_WARNING, &lv_font_montserrat_28);
    lv_obj_align(wifiStatusLabel, LV_ALIGN_RIGHT_MID, -24, 0);

    lv_obj_t* wifiTarget = lv_obj_create(container);
    lv_obj_set_size(wifiTarget, 74, height);
    lv_obj_align(wifiTarget, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_opa(wifiTarget, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(wifiTarget, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(wifiTarget, 0, LV_PART_MAIN);
    lv_obj_add_flag(wifiTarget, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(wifiTarget, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(wifiTarget, settingsEventHandler, LV_EVENT_CLICKED, this);

    lv_obj_t* footer = lv_obj_create(parent);
    lv_obj_set_pos(footer, 0, Theme::FOOTER_TOP);
    lv_obj_set_size(footer, width, Theme::FOOTER_HEIGHT);
    Theme::configureHeader(footer);

    localTimeLabel = Theme::createLabel(footer, "--:--:-- LOCAL", Theme::COLOR_TEXT,
                                        &lv_font_montserrat_20);
    lv_obj_align(localTimeLabel, LV_ALIGN_LEFT_MID, 14, 0);
    dateLabel = Theme::createLabel(footer, "Waiting for time...", Theme::COLOR_TEXT,
                                   &lv_font_montserrat_20);
    lv_obj_align(dateLabel, LV_ALIGN_CENTER, 0, 0);
    utcTimeLabel = Theme::createLabel(footer, "--:--:-- UTC", Theme::COLOR_TEXT,
                                      &lv_font_montserrat_20);
    lv_obj_align(utcTimeLabel, LV_ALIGN_RIGHT_MID, -14, 0);

    createMenu(parent);

    update();
}

void HeaderBar::createMenu(lv_obj_t* parent)
{
    menuOverlay = lv_obj_create(parent);
    lv_obj_set_pos(menuOverlay, 0, Theme::HEADER_HEIGHT);
    lv_obj_set_size(menuOverlay, Theme::SCREEN_WIDTH, Theme::FOOTER_TOP - Theme::HEADER_HEIGHT);
    lv_obj_set_style_bg_color(menuOverlay, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(menuOverlay, LV_OPA_70, LV_PART_MAIN);
    lv_obj_set_style_border_width(menuOverlay, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(menuOverlay, 0, LV_PART_MAIN);
    lv_obj_clear_flag(menuOverlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(menuOverlay, menuOverlayEventHandler, LV_EVENT_CLICKED, this);

    lv_obj_t* menuPanel = lv_obj_create(menuOverlay);
    lv_obj_set_pos(menuPanel, 8, 8);
    lv_obj_set_size(menuPanel, 276, 366);
    Theme::configurePanel(menuPanel);
    lv_obj_t* title = Theme::createLabel(menuPanel, "MISSION CONTROL", Theme::COLOR_PRIMARY);
    lv_obj_set_pos(title, 4, 0);
    createMenuSeparator(menuPanel, 24, LV_OPA_70);

    for (uint8_t index = 0; index < MENU_ITEM_COUNT; ++index)
    {
        lv_obj_t* button = lv_btn_create(menuPanel);
        menuButtons[index] = button;
        const int16_t rowY = index < MENU_ITEM_COUNT - 1
            ? 27 + index * 34
            : 273;
        lv_obj_set_pos(button, 0, rowY);
        lv_obj_set_size(button, 248, 34);
        configureMenuRow(button);
        lv_obj_add_event_cb(button, menuItemEventHandler, LV_EVENT_CLICKED, this);
        lv_obj_t* label = Theme::createLabel(button, MENU_LABELS[index], Theme::COLOR_TEXT);
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 6, 0);

        if (index < MENU_ITEM_COUNT - 2)
            createMenuSeparator(menuPanel, rowY + 33);
    }

    // Separate destination pages from system actions without introducing
    // another boxed control style.
    createMenuSeparator(menuPanel, 268, LV_OPA_70);
    createMenuSeparator(menuPanel, 307);

    screenOffButton = lv_btn_create(menuPanel);
    lv_obj_set_pos(screenOffButton, 0, 308);
    lv_obj_set_size(screenOffButton, 248, 34);
    configureMenuRow(screenOffButton);
    lv_obj_add_event_cb(
        screenOffButton, screenOffEventHandler, LV_EVENT_CLICKED, this);
    lv_obj_t* screenOffLabel = Theme::createLabel(
        screenOffButton,
        LV_SYMBOL_POWER "  TURN OFF SCREEN",
        Theme::COLOR_WARNING);
    lv_obj_align(screenOffLabel, LV_ALIGN_LEFT_MID, 6, 0);

    lv_obj_add_flag(menuOverlay, LV_OBJ_FLAG_HIDDEN);
}

void HeaderBar::setSettingsCallback(SettingsCallback callback)
{
    settingsCallback = callback;
}

void HeaderBar::setNavigationCallback(NavigationCallback callback)
{
    navigationCallback = callback;
}

void HeaderBar::useLocationIdentity(bool enabled)
{
    locationIdentity = enabled;
    update();
}

void HeaderBar::settingsEventHandler(lv_event_t* event)
{
    HeaderBar* self = static_cast<HeaderBar*>(lv_event_get_user_data(event));
    if (self == nullptr) return;
    if (self->settingsCallback != nullptr) self->settingsCallback();
    else if (self->navigationCallback != nullptr) self->navigationCallback(Page::Settings);
}

void HeaderBar::menuEventHandler(lv_event_t* event)
{
    HeaderBar* self = static_cast<HeaderBar*>(lv_event_get_user_data(event));
    if (self == nullptr || self->menuOverlay == nullptr) return;
    if (lv_obj_has_flag(self->menuOverlay, LV_OBJ_FLAG_HIDDEN)) self->showMenu();
    else self->hideMenu();
}

void HeaderBar::menuItemEventHandler(lv_event_t* event)
{
    HeaderBar* self = static_cast<HeaderBar*>(lv_event_get_user_data(event));
    if (self == nullptr) return;
    lv_obj_t* target = lv_event_get_target(event);
    for (uint8_t index = 0; index < MENU_ITEM_COUNT; ++index)
    {
        if (self->menuButtons[index] != target) continue;
        self->hideMenu();
        if (self->navigationCallback != nullptr)
            self->navigationCallback(MENU_PAGES[index]);
        else if (MENU_PAGES[index] == Page::Settings && self->settingsCallback != nullptr)
            self->settingsCallback();
        return;
    }
}

void HeaderBar::screenOffEventHandler(lv_event_t* event)
{
    HeaderBar* self = static_cast<HeaderBar*>(lv_event_get_user_data(event));
    if (self == nullptr) return;
    self->hideMenu();
    if (screenOffCallback != nullptr) screenOffCallback();
}

void HeaderBar::menuOverlayEventHandler(lv_event_t* event)
{
    HeaderBar* self = static_cast<HeaderBar*>(lv_event_get_user_data(event));
    if (self != nullptr && lv_event_get_target(event) == self->menuOverlay) self->hideMenu();
}

void HeaderBar::showMenu()
{
    lv_obj_clear_flag(menuOverlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(menuOverlay);
}

void HeaderBar::hideMenu()
{
    if (menuOverlay != nullptr) lv_obj_add_flag(menuOverlay, LV_OBJ_FLAG_HIDDEN);
}

void HeaderBar::update()
{
    if (clockService == nullptr || identityLabel == nullptr || wifiStatusLabel == nullptr ||
        localTimeLabel == nullptr || dateLabel == nullptr || utcTimeLabel == nullptr) return;

    if (appSettings != nullptr)
    {
        String identity = locationIdentity
            ? appSettings->locationName
            : appSettings->callsign + " - " + appSettings->gridSquare;
        if (identity.isEmpty()) identity = "WEATHER";
        lv_label_set_text(identityLabel, identity.c_str());
    }

    String local = clockService->getLocalTime() + " LOCAL";
    String utc = clockService->getUTCTime() + " UTC";
    lv_label_set_text(localTimeLabel, local.c_str());
    lv_label_set_text(dateLabel, clockService->getDisplayDate().c_str());
    lv_label_set_text(utcTimeLabel, utc.c_str());

    lv_obj_set_style_text_color(
        wifiStatusLabel,
        Theme::color(WiFi.status() == WL_CONNECTED ? Theme::COLOR_SUCCESS : Theme::COLOR_WARNING),
        LV_PART_MAIN);
}
