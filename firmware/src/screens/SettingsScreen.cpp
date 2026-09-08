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

SettingsScreen::BrightnessCallback SettingsScreen::brightnessCallback = nullptr;

void SettingsScreen::configureBrightnessCallback(BrightnessCallback callback)
{
    brightnessCallback = callback;
}

void SettingsScreen::begin(
    ClockService& clockService,
    SettingsService& settingsServiceReference,
    WiFiService& wifiServiceReference,
    LiveSpotsService& liveSpotsServiceReference,
    WsjtxService& wsjtxServiceReference)
{
    if (screen != nullptr) return;

    settingsService = &settingsServiceReference;
    wifiService = &wifiServiceReference;
    liveSpotsService = &liveSpotsServiceReference;
    wsjtxService = &wsjtxServiceReference;

    screen = lv_obj_create(nullptr);
    Theme::configureScreen(screen);
    headerBar.create(screen, clockService, Theme::SCREEN_WIDTH, Theme::HEADER_HEIGHT);
    headerBar.setNavigationCallback([this](Page page) {
        if (navigationCallback != nullptr) navigationCallback(page);
    });

    lv_obj_t* backButton = lv_btn_create(screen);
    lv_obj_set_pos(backButton, 8, Theme::CONTENT_TOP);
    lv_obj_set_size(backButton, 116, 30);
    styleButton(backButton);
    lv_obj_set_style_radius(backButton, 8, LV_PART_MAIN);
    lv_obj_add_event_cb(backButton, backButtonEventHandler, LV_EVENT_CLICKED, this);
    lv_obj_center(Theme::createLabel(backButton, LV_SYMBOL_LEFT " DASHBOARD", Theme::COLOR_PRIMARY));

    lv_obj_t* title = Theme::createLabel(screen, "SYSTEM SETTINGS", Theme::COLOR_PRIMARY);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 140, Theme::CONTENT_TOP + 10);

    displayPageButton = lv_btn_create(screen);
    lv_obj_set_pos(displayPageButton, 286, Theme::CONTENT_TOP);
    lv_obj_set_size(displayPageButton, 126, 30);
    styleButton(displayPageButton);
    lv_obj_set_style_radius(displayPageButton, 8, LV_PART_MAIN);
    lv_obj_add_event_cb(
        displayPageButton, displayPageButtonEventHandler, LV_EVENT_CLICKED, this);
    lv_obj_center(Theme::createLabel(
        displayPageButton, "DISPLAY " LV_SYMBOL_RIGHT, Theme::COLOR_PRIMARY));

    lookupPageButton = lv_btn_create(screen);
    lv_obj_set_pos(lookupPageButton, 420, Theme::CONTENT_TOP);
    lv_obj_set_size(lookupPageButton, 184, 30);
    styleButton(lookupPageButton);
    lv_obj_set_style_radius(lookupPageButton, 8, LV_PART_MAIN);
    lv_obj_add_event_cb(
        lookupPageButton, lookupPageButtonEventHandler, LV_EVENT_CLICKED, this);
    lv_obj_center(Theme::createLabel(
        lookupPageButton, "CALLSIGN LOOKUP", Theme::COLOR_PRIMARY,
        &lv_font_montserrat_12));

    statusLabel = Theme::createLabel(screen, "", Theme::COLOR_TEXT_MUTED);
    lv_obj_set_pos(statusLabel, 612, Theme::CONTENT_TOP + 2);
    lv_obj_set_width(statusLabel, 178);
    lv_obj_set_style_text_font(statusLabel, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_align(statusLabel, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);

    lv_obj_t* panel = Theme::createPanel(screen, 8, 110, 784, 152, "WI-FI AND LOCATION");
    generalPanel = panel;

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

    displayPanel = Theme::createPanel(
        screen, 8, 110, 784, 328, "DISPLAY & STARTUP");
    lv_obj_add_flag(displayPanel, LV_OBJ_FLAG_HIDDEN);

    generalPageButton = lv_btn_create(displayPanel);
    lv_obj_set_pos(generalPageButton, 630, 0);
    lv_obj_set_size(generalPageButton, 126, 30);
    styleButton(generalPageButton);
    lv_obj_set_style_radius(generalPageButton, 8, LV_PART_MAIN);
    lv_obj_add_event_cb(
        generalPageButton, generalPageButtonEventHandler, LV_EVENT_CLICKED, this);
    lv_obj_center(Theme::createLabel(
        generalPageButton, LV_SYMBOL_LEFT " GENERAL", Theme::COLOR_PRIMARY));

    lv_obj_t* description = Theme::createLabel(
        displayPanel,
        "DEFAULT SCREEN\nChoose the screen shown after startup. Detail screens remain memory-efficient and load only when needed.",
        Theme::COLOR_TEXT_MUTED);
    lv_obj_set_pos(description, 0, 38);
    lv_obj_set_width(description, 756);
    lv_label_set_long_mode(description, LV_LABEL_LONG_WRAP);

    const char* defaultScreenNames[7] = {
        "DASHBOARD", "WEATHER", "AIRCRAFT", "SATELLITES",
        "SOLAR", "LIVE SPOTS", "POTA SPOTS"
    };
    for (uint8_t index = 0; index < 7; ++index)
    {
        defaultScreenButtons[index] = lv_btn_create(displayPanel);
        const uint8_t column = index % 4;
        const uint8_t row = index / 4;
        lv_obj_set_pos(defaultScreenButtons[index], column * 190, 108 + row * 76);
        lv_obj_set_size(defaultScreenButtons[index], 178, 62);
        styleButton(defaultScreenButtons[index]);
        lv_obj_add_event_cb(
            defaultScreenButtons[index], defaultScreenButtonEventHandler,
            LV_EVENT_CLICKED, this);
        lv_obj_center(Theme::createLabel(
            defaultScreenButtons[index], defaultScreenNames[index], Theme::COLOR_TEXT));
    }

    lv_obj_t* brightnessLabel = Theme::createLabel(
        displayPanel, "BRIGHTNESS", Theme::COLOR_TEXT_MUTED);
    lv_obj_set_pos(brightnessLabel, 0, 274);

    brightnessSlider = lv_slider_create(displayPanel);
    lv_obj_set_pos(brightnessSlider, 112, 270);
    lv_obj_set_size(brightnessSlider, 550, 18);
    lv_slider_set_range(brightnessSlider, 10, 100);
    lv_obj_set_style_bg_color(
        brightnessSlider, Theme::color(Theme::COLOR_PANEL_BORDER), LV_PART_MAIN);
    lv_obj_set_style_bg_color(
        brightnessSlider, Theme::color(Theme::COLOR_PRIMARY), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(
        brightnessSlider, Theme::color(Theme::COLOR_TEXT), LV_PART_KNOB);
    lv_obj_set_style_pad_all(brightnessSlider, 4, LV_PART_KNOB);
    lv_obj_add_event_cb(
        brightnessSlider, brightnessSliderEventHandler, LV_EVENT_ALL, this);

    brightnessValueLabel = Theme::createLabel(
        displayPanel, "80%", Theme::COLOR_TEXT);
    lv_obj_set_pos(brightnessValueLabel, 682, 272);
    lv_obj_set_width(brightnessValueLabel, 72);
    lv_obj_set_style_text_align(
        brightnessValueLabel, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);

    lookupPanel = Theme::createPanel(
        screen, 8, 110, 784, 328, "CALLSIGN LOOKUP & WSJT-X");
    lv_obj_add_flag(lookupPanel, LV_OBJ_FLAG_HIDDEN);

    lookupGeneralPageButton = lv_btn_create(lookupPanel);
    lv_obj_set_pos(lookupGeneralPageButton, 630, 0);
    lv_obj_set_size(lookupGeneralPageButton, 126, 30);
    styleButton(lookupGeneralPageButton);
    lv_obj_set_style_radius(lookupGeneralPageButton, 8, LV_PART_MAIN);
    lv_obj_add_event_cb(
        lookupGeneralPageButton, generalPageButtonEventHandler, LV_EVENT_CLICKED, this);
    lv_obj_center(Theme::createLabel(
        lookupGeneralPageButton, LV_SYMBOL_LEFT " GENERAL", Theme::COLOR_PRIMARY));

    lv_obj_t* providerLabel = Theme::createLabel(
        lookupPanel, "CALLBOOK", Theme::COLOR_TEXT_MUTED);
    lv_obj_set_pos(providerLabel, 0, 39);
    const char* providerNames[2] = {"HAMQTH", "QRZ"};
    for (uint8_t index = 0; index < 2; ++index)
    {
        lookupProviderButtons[index] = lv_btn_create(lookupPanel);
        lv_obj_set_pos(lookupProviderButtons[index], 88 + index * 112, 30);
        lv_obj_set_size(lookupProviderButtons[index], 102, 38);
        styleButton(lookupProviderButtons[index]);
        lv_obj_add_event_cb(
            lookupProviderButtons[index], lookupProviderButtonEventHandler,
            LV_EVENT_CLICKED, this);
        lv_obj_center(Theme::createLabel(
            lookupProviderButtons[index], providerNames[index], Theme::COLOR_TEXT));
    }

    const char* lookupLabels[4] = {"USERNAME", "PASSWORD", "MULTICAST GROUP", "UDP PORT"};
    const int16_t lookupX[4] = {0, 180, 360, 552};
    const int16_t lookupW[4] = {168, 168, 180, 92};
    lv_obj_t** lookupFields[4] = {
        &lookupUsernameTextArea, &lookupPasswordTextArea,
        &udpGroupTextArea, &udpPortTextArea
    };
    for (uint8_t index = 0; index < 4; ++index)
    {
        lv_obj_t* label = Theme::createLabel(
            lookupPanel, lookupLabels[index], Theme::COLOR_TEXT_MUTED,
            &lv_font_montserrat_12);
        lv_obj_set_pos(label, lookupX[index], 78);
        *lookupFields[index] = lv_textarea_create(lookupPanel);
        lv_obj_set_pos(*lookupFields[index], lookupX[index], 98);
        lv_obj_set_size(*lookupFields[index], lookupW[index], 42);
        lv_textarea_set_one_line(*lookupFields[index], true);
        styleTextArea(*lookupFields[index]);
        lv_obj_add_event_cb(
            *lookupFields[index], textAreaEventHandler, LV_EVENT_PRESSED, this);
    }
    lv_textarea_set_max_length(lookupUsernameTextArea, 32);
    lv_textarea_set_password_mode(lookupPasswordTextArea, true);
    lv_textarea_set_max_length(lookupPasswordTextArea, 64);
    lv_textarea_set_accepted_chars(udpGroupTextArea, "0123456789.");
    lv_textarea_set_max_length(udpGroupTextArea, 15);
    lv_textarea_set_accepted_chars(udpPortTextArea, "0123456789");
    lv_textarea_set_max_length(udpPortTextArea, 5);

    lv_obj_t* lookupSaveButton = lv_btn_create(lookupPanel);
    lv_obj_set_pos(lookupSaveButton, 656, 82);
    lv_obj_set_size(lookupSaveButton, 100, 58);
    styleButton(lookupSaveButton);
    lv_obj_set_style_bg_color(
        lookupSaveButton, Theme::color(0x134A57), LV_PART_MAIN);
    lv_obj_add_event_cb(
        lookupSaveButton, lookupSaveButtonEventHandler, LV_EVENT_CLICKED, this);
    lv_obj_center(Theme::createLabel(
        lookupSaveButton, "SAVE", Theme::COLOR_TEXT));

    lv_obj_t* lookupHelp = Theme::createLabel(
        lookupPanel,
        "Set WSJT-X UDP Server to the same multicast group and port. HamQTH works with a free account; QRZ callbook access requires an eligible XML subscription. Credentials are stored locally on this panel.",
        Theme::COLOR_TEXT_MUTED);
    lv_obj_set_pos(lookupHelp, 0, 162);
    lv_obj_set_width(lookupHelp, 756);
    lv_label_set_long_mode(lookupHelp, LV_LABEL_LONG_WRAP);

    lookupStatusLabel = Theme::createLabel(
        lookupPanel, "", Theme::COLOR_TEXT_MUTED);
    lv_obj_set_pos(lookupStatusLabel, 0, 245);
    lv_obj_set_width(lookupStatusLabel, 756);
    lv_obj_set_style_text_align(
        lookupStatusLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    updateDefaultScreenButtons();

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
    selectedLookupProvider = settings.callsignLookupProvider;
    lv_textarea_set_text(
        lookupUsernameTextArea, settings.callsignLookupUsername.c_str());
    lv_textarea_set_text(
        lookupPasswordTextArea, settings.callsignLookupPassword.c_str());
    lv_textarea_set_text(
        udpGroupTextArea, settings.wsjtxMulticastAddress.c_str());
    lv_textarea_set_text(
        udpPortTextArea, String(settings.wsjtxUdpPort).c_str());
    updateLookupProviderButtons();
    if (brightnessSlider != nullptr)
    {
        const uint8_t brightness = constrain(settings.displayBrightness, 10, 100);
        lv_slider_set_value(brightnessSlider, brightness, LV_ANIM_OFF);
        updateBrightnessLabel();
    }
    lv_scr_load(screen);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(wifiScanOverlay, LV_OBJ_FLAG_HIDDEN);
    showGeneralPage();
    activeTextArea = nullptr;
    updateStatus();
}

void SettingsScreen::release()
{
    if (restartTimer != nullptr) { lv_timer_del(restartTimer); restartTimer = nullptr; }
    if (updateTimer != nullptr) { lv_timer_del(updateTimer); updateTimer = nullptr; }
    if (screen != nullptr) { lv_obj_del(screen); screen = nullptr; }
    ssidTextArea = passwordTextArea = postalCodeTextArea = nullptr;
    gridSquareTextArea = callsignTextArea = keyboard = activeTextArea = nullptr;
    statusLabel = wifiScanOverlay = wifiScanStatusLabel = nullptr;
    generalPanel = displayPanel = lookupPanel = nullptr;
    displayPageButton = lookupPageButton = generalPageButton =
        lookupGeneralPageButton = nullptr;
    lookupUsernameTextArea = lookupPasswordTextArea = udpGroupTextArea =
        udpPortTextArea = lookupStatusLabel = nullptr;
    lookupProviderButtons[0] = lookupProviderButtons[1] = nullptr;
    brightnessSlider = brightnessValueLabel = nullptr;
    for (uint8_t i = 0; i < 7; ++i) defaultScreenButtons[i] = nullptr;
    for (uint8_t i = 0; i < 40; ++i) keyboardButtons[i] = keyboardLabels[i] = nullptr;
    for (uint8_t i = 0; i < 8; ++i)
    {
        wifiNetworkButtons[i] = wifiNetworkLabels[i] = nullptr;
        scannedSsids[i] = "";
    }
    restartPending = false;
}

void SettingsScreen::setNavigationCallback(NavigationCallback callback)
{
    navigationCallback = callback;
}

void SettingsScreen::showDisplayPage()
{
    if (displayPanel == nullptr) return;
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(wifiScanOverlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(generalPanel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(displayPageButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(lookupPageButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(lookupPanel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(displayPanel, LV_OBJ_FLAG_HIDDEN);
    activeTextArea = nullptr;
    updateDefaultScreenButtons();
}

void SettingsScreen::showLookupPage()
{
    if (lookupPanel == nullptr) return;
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(wifiScanOverlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(generalPanel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(displayPanel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(displayPageButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(lookupPageButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(lookupPanel, LV_OBJ_FLAG_HIDDEN);
    activeTextArea = nullptr;
    updateLookupProviderButtons();
}

void SettingsScreen::showGeneralPage()
{
    if (generalPanel == nullptr) return;
    lv_obj_add_flag(displayPanel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(lookupPanel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(generalPanel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(displayPageButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(lookupPageButton, LV_OBJ_FLAG_HIDDEN);
}

void SettingsScreen::updateLookupProviderButtons()
{
    for (uint8_t index = 0; index < 2; ++index)
    {
        if (lookupProviderButtons[index] == nullptr) continue;
        const bool selected = index == selectedLookupProvider;
        lv_obj_set_style_bg_color(
            lookupProviderButtons[index],
            Theme::color(selected ? 0x134A57 : Theme::COLOR_PANEL), LV_PART_MAIN);
        lv_obj_set_style_border_color(
            lookupProviderButtons[index],
            Theme::color(selected ? Theme::COLOR_PRIMARY : Theme::COLOR_PANEL_BORDER),
            LV_PART_MAIN);
        lv_obj_set_style_border_width(
            lookupProviderButtons[index], selected ? 2 : 1, LV_PART_MAIN);
    }
}

void SettingsScreen::saveLookupSettings()
{
    if (settingsService == nullptr) return;
    String username = lv_textarea_get_text(lookupUsernameTextArea);
    String password = lv_textarea_get_text(lookupPasswordTextArea);
    String multicast = lv_textarea_get_text(udpGroupTextArea);
    const uint32_t port = String(lv_textarea_get_text(udpPortTextArea)).toInt();
    username.trim();
    multicast.trim();

    IPAddress group;
    if (!group.fromString(multicast) || group[0] < 224 || group[0] > 239)
    {
        lv_label_set_text(lookupStatusLabel, "ENTER A VALID MULTICAST ADDRESS (224.0.0.0 - 239.255.255.255)");
        lv_obj_set_style_text_color(
            lookupStatusLabel, Theme::color(Theme::COLOR_ERROR), LV_PART_MAIN);
        return;
    }
    if (port == 0 || port > 65535)
    {
        lv_label_set_text(lookupStatusLabel, "ENTER A VALID UDP PORT");
        lv_obj_set_style_text_color(
            lookupStatusLabel, Theme::color(Theme::COLOR_ERROR), LV_PART_MAIN);
        return;
    }

    AppSettings updated = settingsService->get();
    updated.callsignLookupProvider = selectedLookupProvider;
    updated.callsignLookupUsername = username;
    updated.callsignLookupPassword = password;
    updated.wsjtxMulticastAddress = multicast;
    updated.wsjtxUdpPort = static_cast<uint16_t>(port);
    if (!settingsService->save(updated))
    {
        lv_label_set_text(lookupStatusLabel, "UNABLE TO SAVE CALLSIGN LOOKUP SETTINGS");
        lv_obj_set_style_text_color(
            lookupStatusLabel, Theme::color(Theme::COLOR_ERROR), LV_PART_MAIN);
        return;
    }
    if (wsjtxService != nullptr) wsjtxService->configure(settingsService->get());

    const String provider = selectedLookupProvider == 0 ? "HAMQTH" : "QRZ";
    const String message = provider + " SAVED  |  " + multicast + ":" + String(port);
    lv_label_set_text(lookupStatusLabel, message.c_str());
    lv_obj_set_style_text_color(
        lookupStatusLabel, Theme::color(Theme::COLOR_SUCCESS), LV_PART_MAIN);
}

void SettingsScreen::selectDefaultScreen(uint8_t selection)
{
    if (settingsService == nullptr || selection > 6) return;
    AppSettings updated = settingsService->get();
    updated.defaultScreen = selection;
    if (settingsService->save(updated))
    {
        lv_label_set_text(statusLabel, "DEFAULT SCREEN SAVED");
        lv_obj_set_style_text_color(
            statusLabel, Theme::color(Theme::COLOR_SUCCESS), LV_PART_MAIN);
        updateDefaultScreenButtons();
    }
}

void SettingsScreen::updateDefaultScreenButtons()
{
    if (settingsService == nullptr) return;
    const uint8_t selected = settingsService->get().defaultScreen;
    for (uint8_t index = 0; index < 7; ++index)
    {
        if (defaultScreenButtons[index] == nullptr) continue;
        lv_obj_set_style_bg_color(
            defaultScreenButtons[index],
            Theme::color(index == selected ? 0x134A57 : Theme::COLOR_PANEL),
            LV_PART_MAIN);
        lv_obj_set_style_border_color(
            defaultScreenButtons[index],
            Theme::color(index == selected ? Theme::COLOR_PRIMARY : Theme::COLOR_PANEL_BORDER),
            LV_PART_MAIN);
        lv_obj_set_style_border_width(
            defaultScreenButtons[index], index == selected ? 2 : 1, LV_PART_MAIN);
    }
}

void SettingsScreen::updateBrightnessLabel()
{
    if (brightnessSlider == nullptr || brightnessValueLabel == nullptr) return;
    const String value = String(lv_slider_get_value(brightnessSlider)) + "%";
    lv_label_set_text(brightnessValueLabel, value.c_str());
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
    restartTimer = lv_timer_create(restartTimerCallback, 1800, this);
    lv_timer_set_repeat_count(restartTimer, 1);
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
    liveSpotsService->begin(settingsService->get());
    const String message = callsign + " saved - refreshing My Signal Reach";
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

    if (!settingsService->stageWiFiSettings(updated))
    {
        lv_label_set_text(statusLabel, "Unable to stage WiFi settings");
        lv_obj_set_style_text_color(statusLabel, Theme::color(Theme::COLOR_ERROR), LV_PART_MAIN);
        return;
    }

    lv_label_set_text(statusLabel, "APPLYING WIFI\nRESTARTING...");
    lv_obj_set_style_text_color(statusLabel, Theme::color(Theme::COLOR_WARNING), LV_PART_MAIN);
    restartPending = true;
    restartTimer = lv_timer_create(restartTimerCallback, 1800, this);
    lv_timer_set_repeat_count(restartTimer, 1);
}

void SettingsScreen::updateStatus()
{
    if (statusLabel == nullptr) return;
    headerBar.update();
    if (restartPending) return;
    if (settingsService->didWiFiCandidateFail() || wifiService->wereCredentialsRejected())
    {
        lv_label_set_text(statusLabel, "CREDENTIALS REJECTED\nCHECK PASSWORD");
        lv_obj_set_style_text_color(statusLabel, Theme::color(Theme::COLOR_ERROR), LV_PART_MAIN);
    }
    else if (WiFi.status() == WL_CONNECTED)
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
        self->callsignTextArea,
        self->lookupUsernameTextArea,
        self->lookupPasswordTextArea,
        self->udpGroupTextArea,
        self->udpPortTextArea
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
    const bool numeric = activeTextArea == postalCodeTextArea ||
        activeTextArea == udpPortTextArea;
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

void SettingsScreen::displayPageButtonEventHandler(lv_event_t* event)
{
    SettingsScreen* self = static_cast<SettingsScreen*>(lv_event_get_user_data(event));
    if (self != nullptr) self->showDisplayPage();
}

void SettingsScreen::lookupPageButtonEventHandler(lv_event_t* event)
{
    SettingsScreen* self = static_cast<SettingsScreen*>(lv_event_get_user_data(event));
    if (self != nullptr) self->showLookupPage();
}

void SettingsScreen::generalPageButtonEventHandler(lv_event_t* event)
{
    SettingsScreen* self = static_cast<SettingsScreen*>(lv_event_get_user_data(event));
    if (self != nullptr) self->showGeneralPage();
}

void SettingsScreen::lookupProviderButtonEventHandler(lv_event_t* event)
{
    SettingsScreen* self = static_cast<SettingsScreen*>(lv_event_get_user_data(event));
    if (self == nullptr) return;
    lv_obj_t* target = lv_event_get_target(event);
    for (uint8_t index = 0; index < 2; ++index)
    {
        if (self->lookupProviderButtons[index] == target)
        {
            self->selectedLookupProvider = index;
            self->updateLookupProviderButtons();
            return;
        }
    }
}

void SettingsScreen::lookupSaveButtonEventHandler(lv_event_t* event)
{
    SettingsScreen* self = static_cast<SettingsScreen*>(lv_event_get_user_data(event));
    if (self != nullptr) self->saveLookupSettings();
}

void SettingsScreen::defaultScreenButtonEventHandler(lv_event_t* event)
{
    SettingsScreen* self = static_cast<SettingsScreen*>(lv_event_get_user_data(event));
    if (self == nullptr) return;
    lv_obj_t* target = lv_event_get_target(event);
    for (uint8_t index = 0; index < 7; ++index)
    {
        if (self->defaultScreenButtons[index] == target)
        {
            self->selectDefaultScreen(index);
            return;
        }
    }
}

void SettingsScreen::brightnessSliderEventHandler(lv_event_t* event)
{
    SettingsScreen* self = static_cast<SettingsScreen*>(lv_event_get_user_data(event));
    if (self == nullptr || self->brightnessSlider == nullptr) return;

    const lv_event_code_t code = lv_event_get_code(event);
    const uint8_t brightness = static_cast<uint8_t>(
        lv_slider_get_value(self->brightnessSlider));

    if (code == LV_EVENT_VALUE_CHANGED)
    {
        self->updateBrightnessLabel();
        if (brightnessCallback != nullptr) brightnessCallback(brightness);
    }
    else if (code == LV_EVENT_RELEASED && self->settingsService != nullptr)
    {
        AppSettings updated = self->settingsService->get();
        updated.displayBrightness = brightness;
        if (self->settingsService->save(updated))
        {
            lv_label_set_text(self->statusLabel, "BRIGHTNESS SAVED");
            lv_obj_set_style_text_color(
                self->statusLabel, Theme::color(Theme::COLOR_SUCCESS), LV_PART_MAIN);
        }
    }
}
