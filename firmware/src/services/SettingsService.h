#pragma once

#include <Arduino.h>
#include <Preferences.h>

#include "../models/AppSettings.h"

/**
 * Owns all persistent Mission Control settings.
 *
 * Settings are stored in the ESP32's nonvolatile NVS flash storage
 * through the Arduino Preferences library.
 */
class SettingsService
{
public:
    /**
     * Opens the NVS settings namespace and loads saved values.
     *
     * On first startup, or when the stored schema is outdated,
     * default values are loaded and saved automatically.
     */
    bool begin();

    /**
     * Returns the current settings.
     */
    const AppSettings& get() const;

    /**
     * Saves a complete settings object to NVS.
     */
    bool save(const AppSettings& newSettings);

    /**
     * Deletes saved settings and restores the compile-time defaults.
     */
    bool resetToDefaults();

    /**
     * Returns true when both an SSID and password are available.
     */
    bool hasWiFiCredentials() const;

    bool stageWiFiSettings(const AppSettings& candidate);
    bool hasPendingWiFiSettings() const;
    bool commitPendingWiFiSettings();
    bool discardPendingWiFiSettings();
    bool didWiFiCandidateFail() const;
    void clearWiFiCandidateFailure();

private:
    static constexpr const char* NVS_NAMESPACE =
        "missionctrl";

    static constexpr uint16_t CURRENT_SCHEMA_VERSION = 1;

    Preferences preferences;
    AppSettings currentSettings;

    bool initialized = false;
    bool pendingWiFiSettings = false;
    bool wifiCandidateFailed = false;

    void loadDefaults();
    void loadFromStorage();
    void saveToStorage();
};
