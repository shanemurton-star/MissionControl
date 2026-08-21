#include "SettingsScreen.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>

#include "../ui/Theme.h"

namespace
{
    const char* LOWER_KEYS[40] = {
        "1","2","3","4","5","6","7","8","9","0",
        "q","w","e","r","t","y","u","i","o","p",
        "a","s","d","f","g","h","j","k","l",LV_SYMBOL_BACKSPACE,
        "ABC","z","x","c","v","b","n","m",".","SYM"
    };
    const char* UPPER_KEYS[40] = {
        "1","2","3","4","5","6","7","8","9","0",
        "Q","W","E","R","T","Y","U","I","O","P",
        "A","S","D","F","G","H","J","K","L",LV_SYMBOL_BACKSPACE,
        "abc","Z","X","C","V","B","N","M",".","SYM"
    };
    const char* SYMBOL_KEYS[40] = {
        "!","@","#","$","%","^","&","*","(",")",
        "~","`","+","=","{","}","[","]","|","/",
        ":",";","'","\"","<",">","?",",",".",LV_SYMBOL_BACKSPACE,
        "ABC","-","_","\\",":",";","?","!","@","abc"
    };

    void styleButton(lv_obj_t* button)
    {
        lv_obj_set_style_bg_color(button, Theme::color(Theme::COLOR_PANEL), LV_PART_MAIN);
        lv_obj_set_style_border_color(button, Theme::color(Theme::COLOR_PANEL_BORDER), LV_PART_MAIN);
        lv_obj_set_style_border_width(button, 1, LV_PART_MAIN);
    }

    void styleTextArea(lv_obj_t* textArea)
    {
        lv_obj_set_style_bg_color(textArea, Theme::color(Theme::COLOR_PANEL), LV_PART_MAIN);
        lv_obj_set_style_text_color(textArea, Theme::color(Theme::COLOR_TEXT), LV_PART_MAIN);
        lv_obj_set_style_border_color(textArea, Theme::color(Theme::COLOR_PANEL_BORDER), LV_PART_MAIN);
        lv_obj_set_style_border_width(textArea, 1, LV_PART_MAIN);
        lv_obj_set_style_text_font(textArea, &lv_font_montserrat_14, LV_PART_MAIN);
    }
}

void SettingsScreen::begin(
    ClockService& clockService,
    SettingsService& settingsServiceReference,
    WiFiService& wifiServiceReference,
    LiveSpotsService& liveSpotsServiceReference)
{
    if (screen != nullptr) return;

    settingsService = &settingsServiceReference;
    wifiService = &wifiServiceReference;
    liveSpotsService = &liveSpotsServiceReference;

    screen = lv_obj_create(nullptr);
    Theme::configureScreen(screen);
    headerBar.create(screen, clockService, Theme::SCREEN_WIDTH, Theme::HEADER_HEIGHT);
    headerBar.setNavigationCallback([this](Page page) {
        if (navigationCallback != nullptr) navigationCallback(page);
    });

    lv_obj_t* backButton = lv_btn_create(screen);
    lv_obj_set_pos(backButton, 8, Theme::CONTENT_TOP);
    lv_obj_set_size(backButton, 116, 36);
    styleButton(backButton);
    lv_obj_add_event_cb(backButton, backButtonEventHandler, LV_EVENT_CLICKED, this);
    lv_obj_center(Theme::createLabel(backButton, LV_SYMBOL_LEFT " DASHBOARD", Theme::COLOR_PRIMARY));

    lv_obj_t* title = Theme::createLabel(screen, "SYSTEM SETTINGS", Theme::COLOR_PRIMARY);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 140, Theme::CONTENT_TOP + 10);

    statusLabel = Theme::createLabel(screen, "", Theme::COLOR_TEXT_MUTED);
    lv_obj_set_pos(statusLabel, 410, Theme::CONTENT_TOP + 9);
    lv_obj_set_width(statusLabel, 380);
    lv_obj_set_style_text_align(statusLabel, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);

    lv_obj_t* panel = Theme::createPanel(screen, 8, 110, 784, 152, "WI-FI AND LOCATION");

    lv_obj_t* wifiScanButton = lv_btn_create(panel);
    lv_obj_set_pos(wifiScanButton, 0, 25);
    lv_obj_set_size(wifiScanButton, 88, 42);
    styleButton(wifiScanButton);
    lv_obj_add_event_cb(wifiScanButton, wifiScanButtonEventHandler, LV_EVENT_CLICKED, this);
    lv_obj_center(Theme::createLabel(wifiScanButton, "BROWSE", Theme::COLOR_PRIMARY));
    ssidTextArea = lv_textarea_create(panel);
    lv_obj_set_pos(ssidTextArea, 96, 25);
    lv_obj_set_size(ssidTextArea, 220, 42);
    lv_textarea_set_one_line(ssidTextArea, true);
    lv_textarea_set_max_length(ssidTextArea, 32);
    styleTextArea(ssidTextArea);
    lv_obj_add_flag(ssidTextArea, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(ssidTextArea, textAreaEventHandler, LV_EVENT_PRESSED, this);

    lv_obj_t* passwordLabel = Theme::createLabel(panel, "PASSWORD", Theme::COLOR_TEXT_MUTED);
    lv_obj_set_pos(passwordLabel, 0, 86);
    passwordTextArea = lv_textarea_create(panel);
    lv_obj_set_pos(passwordTextArea, 96, 77);
    lv_obj_set_size(passwordTextArea, 220, 42);
    lv_textarea_set_one_line(passwordTextArea, true);
    lv_textarea_set_password_mode(passwordTextArea, true);
    lv_textarea_set_max_length(passwordTextArea, 64);
    styleTextArea(passwordTextArea);
    lv_obj_add_flag(passwordTextArea, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(passwordTextArea, textAreaEventHandler, LV_EVENT_PRESSED, this);

    lv_obj_t* saveButton = lv_btn_create(panel);
    lv_obj_set_pos(saveButton, 330, 25);
    lv_obj_set_size(saveButton, 112, 94);
    styleButton(saveButton);
    lv_obj_set_style_bg_color(saveButton, Theme::color(0x134A57), LV_PART_MAIN);
    lv_obj_add_event_cb(saveButton, saveButtonEventHandler, LV_EVENT_CLICKED, this);
    lv_obj_center(Theme::createLabel(saveButton, "SAVE WIFI", Theme::COLOR_TEXT));

    lv_obj_t* callsignLabel = Theme::createLabel(panel, "CALLSIGN", Theme::COLOR_TEXT_MUTED);
    lv_obj_set_pos(callsignLabel, 462, 2);
    callsignTextArea = lv_textarea_create(panel);
    lv_obj_set_pos(callsignTextArea, 462, 25);
    lv_obj_set_size(callsignTextArea, 92, 42);
    lv_textarea_set_one_line(callsignTextArea, true);
    lv_textarea_set_accepted_chars(callsignTextArea, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789/");
    lv_textarea_set_max_length(callsignTextArea, 12);
    lv_textarea_set_placeholder_text(callsignTextArea, "KF8EFV");
    styleTextArea(callsignTextArea);
    lv_obj_add_flag(callsignTextArea, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(callsignTextArea, textAreaEventHandler, LV_EVENT_PRESSED, this);

    lv_obj_t* callsignButton = lv_btn_create(panel);
    lv_obj_set_pos(callsignButton, 462, 77);
    lv_obj_set_size(callsignButton, 92, 42);
    styleButton(callsignButton);
    lv_obj_set_style_bg_color(callsignButton, Theme::color(0x134A57), LV_PART_MAIN);
    lv_obj_add_event_cb(callsignButton, callsignButtonEventHandler, LV_EVENT_CLICKED, this);
    lv_obj_center(Theme::createLabel(callsignButton, "APPLY", Theme::COLOR_TEXT));

    lv_obj_t* gridLabel = Theme::createLabel(panel, "GRID", Theme::COLOR_TEXT_MUTED);
    lv_obj_set_pos(gridLabel, 562, 2);
    gridSquareTextArea = lv_textarea_create(panel);
    lv_obj_set_pos(gridSquareTextArea, 562, 25);
    lv_obj_set_size(gridSquareTextArea, 88, 42);
    lv_textarea_set_one_line(gridSquareTextArea, true);
    lv_textarea_set_accepted_chars(
        gridSquareTextArea,
        "ABCDEFGHIJKLMNOPQRSTUVWXabcdefghijklmnopqrstuvwx0123456789");
    lv_textarea_set_max_length(gridSquareTextArea, 6);
    lv_textarea_set_placeholder_text(gridSquareTextArea, "EN82fs");
    styleTextArea(gridSquareTextArea);
    lv_obj_add_flag(gridSquareTextArea, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(gridSquareTextArea, textAreaEventHandler, LV_EVENT_PRESSED, this);

    lv_obj_t* gridButton = lv_btn_create(panel);
    lv_obj_set_pos(gridButton, 562, 77);
    lv_obj_set_size(gridButton, 88, 42);
    styleButton(gridButton);
    lv_obj_set_style_bg_color(gridButton, Theme::color(0x134A57), LV_PART_MAIN);
    lv_obj_add_event_cb(gridButton, gridButtonEventHandler, LV_EVENT_CLICKED, this);
    lv_obj_center(Theme::createLabel(gridButton, "APPLY", Theme::COLOR_TEXT));

    lv_obj_t* postalLabel = Theme::createLabel(panel, "ZIP CODE", Theme::COLOR_TEXT_MUTED);
    lv_obj_set_pos(postalLabel, 658, 2);
    postalCodeTextArea = lv_textarea_create(panel);
    lv_obj_set_pos(postalCodeTextArea, 658, 25);
    lv_obj_set_size(postalCodeTextArea, 97, 42);
    lv_textarea_set_one_line(postalCodeTextArea, true);
    lv_textarea_set_accepted_chars(postalCodeTextArea, "0123456789");
    lv_textarea_set_max_length(postalCodeTextArea, 5);
    lv_textarea_set_placeholder_text(postalCodeTextArea, "5-DIGIT ZIP");
    styleTextArea(postalCodeTextArea);
    lv_obj_add_flag(postalCodeTextArea, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(postalCodeTextArea, textAreaEventHandler, LV_EVENT_PRESSED, this);

    lv_obj_t* locationButton = lv_btn_create(panel);
    lv_obj_set_pos(locationButton, 658, 77);
    lv_obj_set_size(locationButton, 97, 42);
    styleButton(locationButton);
    lv_obj_set_style_bg_color(locationButton, Theme::color(0x134A57), LV_PART_MAIN);
    lv_obj_add_event_cb(locationButton, locationButtonEventHandler, LV_EVENT_CLICKED, this);
    lv_obj_center(Theme::createLabel(locationButton, "APPLY", Theme::COLOR_TEXT));

    keyboard = lv_obj_create(screen);
    lv_obj_set_pos(keyboard, 8, 270);
    lv_obj_set_size(keyboard, 784, 168);
    lv_obj_set_style_bg_color(keyboard, Theme::color(Theme::COLOR_PANEL), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(keyboard, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_opa(keyboard, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(keyboard, Theme::color(Theme::COLOR_PANEL_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(keyboard, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(keyboard, 0, LV_PART_MAIN);
    lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_SCROLLABLE);

    for (uint8_t index = 0; index < 40; ++index)
    {
        keyboardButtons[index] = lv_btn_create(keyboard);
        lv_obj_set_pos(keyboardButtons[index], 5 + (index % 10) * 77, 4 + (index / 10) * 40);
        lv_obj_set_size(keyboardButtons[index], 72, 36);
        styleButton(keyboardButtons[index]);
        lv_obj_set_style_bg_color(keyboardButtons[index], Theme::color(0x13283A), LV_PART_MAIN);
        lv_obj_set_style_bg_color(
            keyboardButtons[index], Theme::color(Theme::COLOR_PRIMARY), LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_add_event_cb(
            keyboardButtons[index], keyboardButtonEventHandler, LV_EVENT_CLICKED, this);
        keyboardLabels[index] = Theme::createLabel(keyboardButtons[index], "", Theme::COLOR_TEXT);
        lv_obj_center(keyboardLabels[index]);
    }
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);

    wifiScanOverlay = Theme::createPanel(screen, 8, 110, 784, 328, "AVAILABLE WI-FI NETWORKS");
    lv_obj_add_flag(wifiScanOverlay, LV_OBJ_FLAG_HIDDEN);
    wifiScanStatusLabel = Theme::createLabel(
        wifiScanOverlay, "Touch BROWSE to scan", Theme::COLOR_TEXT_MUTED);
    lv_obj_set_pos(wifiScanStatusLabel, 0, 30);

    for (uint8_t index = 0; index < 8; ++index)
    {
        wifiNetworkButtons[index] = lv_btn_create(wifiScanOverlay);
        lv_obj_set_pos(wifiNetworkButtons[index], (index % 2) * 380, 55 + (index / 2) * 52);
        lv_obj_set_size(wifiNetworkButtons[index], 366, 44);
        styleButton(wifiNetworkButtons[index]);
        lv_obj_add_event_cb(
            wifiNetworkButtons[index], wifiNetworkEventHandler, LV_EVENT_CLICKED, this);
        wifiNetworkLabels[index] = Theme::createLabel(
            wifiNetworkButtons[index], "", Theme::COLOR_TEXT);
        lv_obj_align(wifiNetworkLabels[index], LV_ALIGN_LEFT_MID, 4, 0);
        lv_obj_add_flag(wifiNetworkButtons[index], LV_OBJ_FLAG_HIDDEN);
    }

    lv_obj_t* closeScanButton = lv_btn_create(wifiScanOverlay);
    lv_obj_set_pos(closeScanButton, 650, 5);
    lv_obj_set_size(closeScanButton, 104, 38);
    styleButton(closeScanButton);
    lv_obj_add_event_cb(closeScanButton, wifiScanCloseEventHandler, LV_EVENT_CLICKED, this);
    lv_obj_center(Theme::createLabel(closeScanButton, "CLOSE", Theme::COLOR_TEXT));

    updateStatus();
    updateTimer = lv_timer_create(updateTimerCallback, 1000, this);
}

void SettingsScreen::show()
{
    if (screen == nullptr || settingsService == nullptr) return;
    const AppSettings& settings = settingsService->get();
    lv_textarea_set_text(ssidTextArea, settings.wifiSsid.c_str());
    lv_textarea_set_text(passwordTextArea, settings.wifiPassword.c_str());
    lv_textarea_set_text(postalCodeTextArea, settings.postalCode.c_str());
    lv_textarea_set_text(gridSquareTextArea, settings.gridSquare.c_str());
    lv_textarea_set_text(callsignTextArea, settings.callsign.c_str());
    lv_scr_load(screen);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(wifiScanOverlay, LV_OBJ_FLAG_HIDDEN);
    activeTextArea = nullptr;
    updateStatus();
}

void SettingsScreen::setNavigationCallback(NavigationCallback callback)
{
    navigationCallback = callback;
}

void SettingsScreen::saveLocation()
{
    const String postalCode = lv_textarea_get_text(postalCodeTextArea);
    if (postalCode.length() != 5)
    {
        lv_label_set_text(statusLabel, "Enter a valid 5-digit U.S. ZIP code");
        lv_obj_set_style_text_color(statusLabel, Theme::color(Theme::COLOR_ERROR), LV_PART_MAIN);
        return;
    }
    if (WiFi.status() != WL_CONNECTED)
    {
        lv_label_set_text(statusLabel, "Wi-Fi is required to look up a ZIP code");
        lv_obj_set_style_text_color(statusLabel, Theme::color(Theme::COLOR_ERROR), LV_PART_MAIN);
        return;
    }

    lv_label_set_text(statusLabel, "Looking up ZIP code...");
    lv_obj_set_style_text_color(statusLabel, Theme::color(Theme::COLOR_WARNING), LV_PART_MAIN);
    lv_timer_handler();

    HTTPClient http;
    http.setConnectTimeout(8000);
    http.setTimeout(8000);
    http.setUserAgent("MissionControl-ESP32/1.0");
    const String url = "https://api.zippopotam.us/us/" + postalCode;
    if (!http.begin(url))
    {
        lv_label_set_text(statusLabel, "Unable to start ZIP lookup");
        lv_obj_set_style_text_color(statusLabel, Theme::color(Theme::COLOR_ERROR), LV_PART_MAIN);
        return;
    }

    const int responseCode = http.GET();
    if (responseCode != HTTP_CODE_OK)
    {
        http.end();
        lv_label_set_text(statusLabel,
            responseCode == HTTP_CODE_NOT_FOUND ? "ZIP code not found" : "ZIP lookup failed");
        lv_obj_set_style_text_color(statusLabel, Theme::color(Theme::COLOR_ERROR), LV_PART_MAIN);
        return;
    }

    JsonDocument document;
    const DeserializationError error = deserializeJson(document, http.getStream());
    http.end();
    JsonObject place = document["places"][0];
    if (error || place.isNull())
    {
        lv_label_set_text(statusLabel, "Invalid ZIP lookup response");
        lv_obj_set_style_text_color(statusLabel, Theme::color(Theme::COLOR_ERROR), LV_PART_MAIN);
        return;
    }

    const char* city = place["place name"] | "";
    const char* state = place["state abbreviation"] | "";
    const double latitude = String(place["latitude"] | "0").toDouble();
    const double longitude = String(place["longitude"] | "0").toDouble();
    if (city[0] == '\0' || latitude == 0.0 || longitude == 0.0)
    {
        lv_label_set_text(statusLabel, "ZIP lookup did not include coordinates");
        lv_obj_set_style_text_color(statusLabel, Theme::color(Theme::COLOR_ERROR), LV_PART_MAIN);
        return;
    }

    AppSettings updated = settingsService->get();
    updated.postalCode = postalCode;
    updated.locationName = String(city) + ", " + state;
    updated.latitude = latitude;
    updated.longitude = longitude;
    if (!settingsService->save(updated))
    {
        lv_label_set_text(statusLabel, "Unable to save location");
        lv_obj_set_style_text_color(statusLabel, Theme::color(Theme::COLOR_ERROR), LV_PART_MAIN);
        return;
    }

    const String message = updated.locationName + " saved - restarting...";
    lv_label_set_text(statusLabel, message.c_str());
    lv_obj_set_style_text_color(statusLabel, Theme::color(Theme::COLOR_SUCCESS), LV_PART_MAIN);
    restartPending = true;
    lv_timer_t* timer = lv_timer_create(restartTimerCallback, 1800, this);
    lv_timer_set_repeat_count(timer, 1);
}

void SettingsScreen::saveGridSquare()
{
    String grid = lv_textarea_get_text(gridSquareTextArea);
    grid.trim();
    if (grid.length() >= 2)
    {
        String field = grid.substring(0, 2);
        field.toUpperCase();
        grid = field + grid.substring(2);
    }
    if (grid.length() == 6)
    {
        String subsquare = grid.substring(4, 6);
        subsquare.toLowerCase();
        grid = grid.substring(0, 4) + subsquare;
    }

    const bool validLength = grid.length() == 4 || grid.length() == 6;
    const bool validField = validLength &&
        grid[0] >= 'A' && grid[0] <= 'R' &&
        grid[1] >= 'A' && grid[1] <= 'R' &&
        grid[2] >= '0' && grid[2] <= '9' &&
        grid[3] >= '0' && grid[3] <= '9';
    const bool validSubsquare = grid.length() != 6 ||
        (grid[4] >= 'a' && grid[4] <= 'x' && grid[5] >= 'a' && grid[5] <= 'x');
    if (!validField || !validSubsquare)
    {
        lv_label_set_text(statusLabel, "Grid must look like EN82 or EN82fs");
        lv_obj_set_style_text_color(statusLabel, Theme::color(Theme::COLOR_ERROR), LV_PART_MAIN);
        return;
    }

    AppSettings updated = settingsService->get();
    updated.gridSquare = grid;
    if (!settingsService->save(updated))
    {
        lv_label_set_text(statusLabel, "Unable to save grid square");
        lv_obj_set_style_text_color(statusLabel, Theme::color(Theme::COLOR_ERROR), LV_PART_MAIN);
        return;
    }

    lv_textarea_set_text(gridSquareTextArea, grid.c_str());
    liveSpotsService->begin(settingsService->get());
    const String message = grid + " saved - refreshing Live Spots";
    lv_label_set_text(statusLabel, message.c_str());
    lv_obj_set_style_text_color(statusLabel, Theme::color(Theme::COLOR_SUCCESS), LV_PART_MAIN);
}

void SettingsScreen::saveCallsign()
{
    String callsign = lv_textarea_get_text(callsignTextArea);
    callsign.trim();
    callsign.toUpperCase();
    if (callsign.length() < 3 || callsign.length() > 12)
    {
        lv_label_set_text(statusLabel, "Enter a valid callsign");
        lv_obj_set_style_text_color(statusLabel, Theme::color(Theme::COLOR_ERROR), LV_PART_MAIN);
        return;
    }

    AppSettings updated = settingsService->get();
    updated.callsign = callsign;
    if (!settingsService->save(updated))
    {
        lv_label_set_text(statusLabel, "Unable to save callsign");
        lv_obj_set_style_text_color(statusLabel, Theme::color(Theme::COLOR_ERROR), LV_PART_MAIN);
        return;
    }

    lv_textarea_set_text(callsignTextArea, callsign.c_str());
    headerBar.update();
    const String message = callsign + " saved";
    lv_label_set_text(statusLabel, message.c_str());
    lv_obj_set_style_text_color(statusLabel, Theme::color(Theme::COLOR_SUCCESS), LV_PART_MAIN);
}

void SettingsScreen::save()
{
    const char* ssid = lv_textarea_get_text(ssidTextArea);
    const char* password = lv_textarea_get_text(passwordTextArea);
    if (ssid == nullptr || ssid[0] == '\0' || password == nullptr || password[0] == '\0')
    {
        lv_label_set_text(statusLabel, "SSID and password are required");
        lv_obj_set_style_text_color(statusLabel, Theme::color(Theme::COLOR_ERROR), LV_PART_MAIN);
        return;
    }

    AppSettings updated = settingsService->get();
    updated.wifiSsid = ssid;
    updated.wifiPassword = password;

    if (!settingsService->save(updated))
    {
        lv_label_set_text(statusLabel, "Unable to save settings");
        lv_obj_set_style_text_color(statusLabel, Theme::color(Theme::COLOR_ERROR), LV_PART_MAIN);
        return;
    }

    lv_label_set_text(statusLabel, "Saved\nConnecting...");
    lv_obj_set_style_text_color(statusLabel, Theme::color(Theme::COLOR_WARNING), LV_PART_MAIN);
    wifiService->begin(settingsService->get());
}

void SettingsScreen::updateStatus()
{
    if (statusLabel == nullptr) return;
    headerBar.update();
    if (restartPending) return;
    if (WiFi.status() == WL_CONNECTED)
    {
        const String status = "CONNECTED\n" + WiFi.localIP().toString() + "\n" + String(WiFi.RSSI()) + " dBm";
        lv_label_set_text(statusLabel, status.c_str());
        lv_obj_set_style_text_color(statusLabel, Theme::color(Theme::COLOR_SUCCESS), LV_PART_MAIN);
    }
    else
    {
        lv_label_set_text(statusLabel, "NOT CONNECTED");
        lv_obj_set_style_text_color(statusLabel, Theme::color(Theme::COLOR_WARNING), LV_PART_MAIN);
    }
}

void SettingsScreen::backButtonEventHandler(lv_event_t* event)
{
    SettingsScreen* self = static_cast<SettingsScreen*>(lv_event_get_user_data(event));
    if (self != nullptr && self->navigationCallback != nullptr)
        self->navigationCallback(Page::Dashboard);
}

void SettingsScreen::saveButtonEventHandler(lv_event_t* event)
{
    SettingsScreen* self = static_cast<SettingsScreen*>(lv_event_get_user_data(event));
    if (self != nullptr) self->save();
}

void SettingsScreen::locationButtonEventHandler(lv_event_t* event)
{
    SettingsScreen* self = static_cast<SettingsScreen*>(lv_event_get_user_data(event));
    if (self != nullptr) self->saveLocation();
}

void SettingsScreen::gridButtonEventHandler(lv_event_t* event)
{
    SettingsScreen* self = static_cast<SettingsScreen*>(lv_event_get_user_data(event));
    if (self != nullptr) self->saveGridSquare();
}

void SettingsScreen::callsignButtonEventHandler(lv_event_t* event)
{
    SettingsScreen* self = static_cast<SettingsScreen*>(lv_event_get_user_data(event));
    if (self != nullptr) self->saveCallsign();
}

void SettingsScreen::wifiScanButtonEventHandler(lv_event_t* event)
{
    SettingsScreen* self = static_cast<SettingsScreen*>(lv_event_get_user_data(event));
    if (self != nullptr) self->scanForWiFi();
}

void SettingsScreen::wifiNetworkEventHandler(lv_event_t* event)
{
    SettingsScreen* self = static_cast<SettingsScreen*>(lv_event_get_user_data(event));
    if (self == nullptr) return;
    lv_obj_t* target = lv_event_get_target(event);
    for (uint8_t index = 0; index < 8; ++index)
    {
        if (self->wifiNetworkButtons[index] == target && !self->scannedSsids[index].isEmpty())
        {
            lv_textarea_set_text(self->ssidTextArea, self->scannedSsids[index].c_str());
            lv_obj_add_flag(self->wifiScanOverlay, LV_OBJ_FLAG_HIDDEN);
            self->showKeyboard(self->passwordTextArea);
            return;
        }
    }
}

void SettingsScreen::wifiScanCloseEventHandler(lv_event_t* event)
{
    SettingsScreen* self = static_cast<SettingsScreen*>(lv_event_get_user_data(event));
    if (self != nullptr) lv_obj_add_flag(self->wifiScanOverlay, LV_OBJ_FLAG_HIDDEN);
}

void SettingsScreen::scanForWiFi()
{
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(wifiScanOverlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(wifiScanOverlay);
    lv_label_set_text(wifiScanStatusLabel, "Scanning...");
    for (uint8_t index = 0; index < 8; ++index)
    {
        scannedSsids[index] = "";
        lv_obj_add_flag(wifiNetworkButtons[index], LV_OBJ_FLAG_HIDDEN);
    }
    lv_timer_handler();

    const int found = wifiService->scanNetworks();
    uint8_t shown = 0;
    for (int source = 0; source < found && shown < 8; ++source)
    {
        const String ssid = WiFi.SSID(source);
        if (ssid.isEmpty()) continue;
        bool duplicate = false;
        for (uint8_t previous = 0; previous < shown; ++previous)
            if (scannedSsids[previous] == ssid) duplicate = true;
        if (duplicate) continue;

        scannedSsids[shown] = ssid;
        const bool secured = WiFi.encryptionType(source) != WIFI_AUTH_OPEN;
        String label = ssid + "   " + String(WiFi.RSSI(source)) + " dBm";
        if (secured)
        {
            label += "  ";
            label += "SECURE";
        }
        lv_label_set_text(wifiNetworkLabels[shown], label.c_str());
        lv_obj_clear_flag(wifiNetworkButtons[shown], LV_OBJ_FLAG_HIDDEN);
        ++shown;
    }
    wifiService->finishNetworkScan();
    if (found < 0)
    {
        const String error = String("WiFi scan failed: ") + found +
            " - manual entry remains available";
        lv_label_set_text(wifiScanStatusLabel, error.c_str());
    }
    else
    {
        lv_label_set_text(
            wifiScanStatusLabel,
            shown == 0 ? "No 2.4 GHz networks visible - manual entry remains available" :
                         "Select a network, then enter its password");
    }
}

void SettingsScreen::textAreaEventHandler(lv_event_t* event)
{
    SettingsScreen* self = static_cast<SettingsScreen*>(lv_event_get_user_data(event));
    if (self == nullptr) return;

    lv_obj_t* target = lv_event_get_target(event);
    self->showKeyboard(target);

    lv_obj_t* fields[] = {
        self->ssidTextArea,
        self->passwordTextArea,
        self->postalCodeTextArea,
        self->gridSquareTextArea,
        self->callsignTextArea
    };
    for (lv_obj_t* field : fields)
    {
        lv_obj_set_style_border_color(
            field,
            Theme::color(field == target ? Theme::COLOR_PRIMARY : Theme::COLOR_PANEL_BORDER),
            LV_PART_MAIN);
        lv_obj_set_style_border_width(field, field == target ? 2 : 1, LV_PART_MAIN);
    }
}

void SettingsScreen::showKeyboard(lv_obj_t* textArea)
{
    activeTextArea = textArea;
    uppercaseKeyboard = false;
    symbolKeyboard = false;
    updateKeyboardKeys();
    lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(keyboard);
    lv_obj_invalidate(keyboard);
}

void SettingsScreen::updateKeyboardKeys()
{
    const bool numeric = activeTextArea == postalCodeTextArea;
    const char** keys = symbolKeyboard ? SYMBOL_KEYS :
        (uppercaseKeyboard ? UPPER_KEYS : LOWER_KEYS);

    for (uint8_t index = 0; index < 40; ++index)
    {
        const char* text = keys[index];
        if (numeric)
        {
            text = index < 10 ? LOWER_KEYS[index] :
                (index == 10 ? LV_SYMBOL_BACKSPACE : "");
        }
        lv_label_set_text(keyboardLabels[index], text);
        if (text[0] == '\0') lv_obj_add_flag(keyboardButtons[index], LV_OBJ_FLAG_HIDDEN);
        else lv_obj_clear_flag(keyboardButtons[index], LV_OBJ_FLAG_HIDDEN);
    }
}

void SettingsScreen::keyboardButtonEventHandler(lv_event_t* event)
{
    SettingsScreen* self = static_cast<SettingsScreen*>(lv_event_get_user_data(event));
    if (self == nullptr || self->activeTextArea == nullptr) return;

    lv_obj_t* button = lv_event_get_target(event);
    uint8_t index = 0;
    while (index < 40 && self->keyboardButtons[index] != button) ++index;
    if (index >= 40) return;

    const char* key = lv_label_get_text(self->keyboardLabels[index]);
    if (strcmp(key, LV_SYMBOL_BACKSPACE) == 0)
        lv_textarea_del_char(self->activeTextArea);
    else if (strcmp(key, "ABC") == 0 || strcmp(key, "abc") == 0)
    {
        self->symbolKeyboard = false;
        self->uppercaseKeyboard = !self->uppercaseKeyboard;
        self->updateKeyboardKeys();
    }
    else if (strcmp(key, "SYM") == 0)
    {
        self->symbolKeyboard = true;
        self->updateKeyboardKeys();
    }
    else
        lv_textarea_add_text(self->activeTextArea, key);
}

void SettingsScreen::updateTimerCallback(lv_timer_t* timer)
{
    SettingsScreen* self = static_cast<SettingsScreen*>(timer->user_data);
    if (self != nullptr) self->updateStatus();
}

void SettingsScreen::restartTimerCallback(lv_timer_t* timer)
{
    (void)timer;
    ESP.restart();
}
