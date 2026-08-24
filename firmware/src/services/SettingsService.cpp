#include "SettingsService.h"

#include "../config/Settings.h"
#include "../config/secrets.h"

bool SettingsService::begin()
{
    if (initialized)
    {
        return true;
    }

    Serial.println();
    Serial.println("[SettingsService] Starting...");

    if (!preferences.begin(
            NVS_NAMESPACE,
            false))
    {
        Serial.println(
            "[SettingsService] Unable to open NVS namespace");

        return false;
    }

    initialized = true;

    const uint16_t storedSchema =
        preferences.getUShort(
            "schema",
            0);

    if (storedSchema != CURRENT_SCHEMA_VERSION)
    {
        Serial.println(
            "[SettingsService] No compatible saved settings found");

        Serial.println(
            "[SettingsService] Loading defaults");

        preferences.clear();

        loadDefaults();
        saveToStorage();
    }
    else
    {
        loadFromStorage();

        Serial.println(
            "[SettingsService] Saved settings loaded");
    }

    Serial.print(
        "[SettingsService] Station: ");

    Serial.println(
        currentSettings.stationName);

    Serial.print(
        "[SettingsService] Location: ");

    Serial.println(
        currentSettings.locationName);

    Serial.print(
        "[SettingsService] Wi-Fi configured: ");

    Serial.println(
        hasWiFiCredentials()
            ? "Yes"
            : "No");

    return true;
}

const AppSettings& SettingsService::get() const
{
    return currentSettings;
}

bool SettingsService::save(
    const AppSettings& newSettings)
{
    if (!initialized)
    {
        Serial.println(
            "[SettingsService] Cannot save before begin()");

        return false;
    }

    currentSettings = newSettings;

    saveToStorage();

    Serial.println(
        "[SettingsService] Settings saved");

    return true;
}

bool SettingsService::resetToDefaults()
{
    if (!initialized)
    {
        Serial.println(
            "[SettingsService] Cannot reset before begin()");

        return false;
    }

    Serial.println(
        "[SettingsService] Restoring defaults");

    preferences.clear();

    loadDefaults();
    saveToStorage();

    return true;
}

bool SettingsService::hasWiFiCredentials() const
{
    return
        !currentSettings.wifiSsid.isEmpty() &&
        !currentSettings.wifiPassword.isEmpty();
}

bool SettingsService::stageWiFiSettings(const AppSettings& candidate)
{
    if (!initialized || candidate.wifiSsid.isEmpty() ||
        candidate.wifiPassword.isEmpty()) return false;

    preferences.putString("pendingssid", candidate.wifiSsid);
    preferences.putString("pendingpass", candidate.wifiPassword);
    preferences.putString("pendinghost", candidate.hostname);
    preferences.putBool("wifipending", true);
    preferences.putBool("wififailed", false);
    pendingWiFiSettings = true;
    wifiCandidateFailed = false;
    Serial.println("[SettingsService] Candidate WiFi settings staged for reboot test");
    return true;
}

bool SettingsService::hasPendingWiFiSettings() const
{
    return pendingWiFiSettings;
}

bool SettingsService::commitPendingWiFiSettings()
{
    if (!initialized || !pendingWiFiSettings) return false;
    preferences.putString("ssid", currentSettings.wifiSsid);
    preferences.putString("wifipass", currentSettings.wifiPassword);
    preferences.putString("hostname", currentSettings.hostname);
    preferences.remove("pendingssid");
    preferences.remove("pendingpass");
    preferences.remove("pendinghost");
    preferences.putBool("wifipending", false);
    preferences.putBool("wififailed", false);
    pendingWiFiSettings = false;
    wifiCandidateFailed = false;
    Serial.println("[SettingsService] Candidate WiFi settings verified and committed");
    return true;
}

bool SettingsService::discardPendingWiFiSettings()
{
    if (!initialized || !pendingWiFiSettings) return false;
    preferences.remove("pendingssid");
    preferences.remove("pendingpass");
    preferences.remove("pendinghost");
    preferences.putBool("wifipending", false);
    preferences.putBool("wififailed", true);
    pendingWiFiSettings = false;
    wifiCandidateFailed = true;
    currentSettings.wifiSsid = preferences.getString("ssid", "");
    currentSettings.wifiPassword = preferences.getString("wifipass", "");
    currentSettings.hostname = preferences.getString("hostname", "mission-control");
    Serial.println("[SettingsService] Candidate rejected; restored verified WiFi settings");
    return true;
}

bool SettingsService::didWiFiCandidateFail() const
{
    return wifiCandidateFailed;
}

void SettingsService::clearWiFiCandidateFailure()
{
    if (!initialized || !wifiCandidateFailed) return;
    preferences.putBool("wififailed", false);
    wifiCandidateFailed = false;
    Serial.println("[SettingsService] Verified fallback connected; WiFi failure notice cleared");
}

void SettingsService::loadDefaults()
{
    // --------------------------------------------------------
    // Station
    // --------------------------------------------------------

    currentSettings.callsign =
        DEFAULT_CALLSIGN;

    currentSettings.gridSquare =
        DEFAULT_GRID_SQUARE;

    currentSettings.stationName =
        DEFAULT_STATION_NAME;


    // --------------------------------------------------------
    // Location
    // --------------------------------------------------------

    currentSettings.locationName =
        LOCATION_NAME;

    currentSettings.postalCode = "";

    currentSettings.latitude =
        DEFAULT_LATITUDE;

    currentSettings.longitude =
        DEFAULT_LONGITUDE;


    // --------------------------------------------------------
    // Time
    // --------------------------------------------------------

    currentSettings.timezone =
        TIMEZONE;

    currentSettings.use24HourTime =
        DEFAULT_USE_24_HOUR_TIME;


    // --------------------------------------------------------
    // Network
    //
    // During development, secrets.h supplies first-start
    // credentials. Later, touchscreen setup will replace them.
    // --------------------------------------------------------

    currentSettings.wifiSsid =
        WIFI_SSID;

    currentSettings.wifiPassword =
        WIFI_PASSWORD;

    currentSettings.hostname =
        DEFAULT_HOSTNAME;


    // --------------------------------------------------------
    // Display
    // --------------------------------------------------------

    currentSettings.displayBrightness =
        DISPLAY_BRIGHTNESS;

    currentSettings.defaultScreen = 0;


    // --------------------------------------------------------
    // Weather
    // --------------------------------------------------------

    currentSettings.weatherRefreshMinutes =
        DEFAULT_WEATHER_REFRESH_MINUTES;

    currentSettings.useFahrenheit =
        DEFAULT_USE_FAHRENHEIT;
}

void SettingsService::loadFromStorage()
{
    // Load defaults first so any future setting that is absent
    // from older saved data still receives a sensible value.
    loadDefaults();


    // --------------------------------------------------------
    // Station
    // --------------------------------------------------------

    currentSettings.callsign =
        preferences.getString(
            "callsign",
            currentSettings.callsign);

    // Early builds stored an empty callsign. Treat that as an unset value
    // so the station identity is always visible until the user changes it.
    if (currentSettings.callsign.isEmpty())
    {
        currentSettings.callsign = DEFAULT_CALLSIGN;
    }

    currentSettings.gridSquare =
        preferences.getString(
            "grid",
            currentSettings.gridSquare);

    currentSettings.stationName =
        preferences.getString(
            "station",
            currentSettings.stationName);


    // --------------------------------------------------------
    // Location
    // --------------------------------------------------------

    currentSettings.locationName =
        preferences.getString(
            "location",
            currentSettings.locationName);

    currentSettings.postalCode =
        preferences.getString(
            "postal",
            currentSettings.postalCode);

    currentSettings.latitude =
        preferences.getDouble(
            "latitude",
            currentSettings.latitude);

    currentSettings.longitude =
        preferences.getDouble(
            "longitude",
            currentSettings.longitude);


    // --------------------------------------------------------
    // Time
    // --------------------------------------------------------

    currentSettings.timezone =
        preferences.getString(
            "timezone",
            currentSettings.timezone);

    currentSettings.use24HourTime =
        preferences.getBool(
            "use24hour",
            currentSettings.use24HourTime);


    // --------------------------------------------------------
    // Network
    // --------------------------------------------------------

    currentSettings.wifiSsid =
        preferences.getString(
            "ssid",
            currentSettings.wifiSsid);

    currentSettings.wifiPassword =
        preferences.getString(
            "wifipass",
            currentSettings.wifiPassword);

    currentSettings.hostname =
        preferences.getString(
            "hostname",
            currentSettings.hostname);

    pendingWiFiSettings = preferences.getBool("wifipending", false);
    wifiCandidateFailed = preferences.getBool("wififailed", false);
    if (pendingWiFiSettings)
    {
        currentSettings.wifiSsid = preferences.getString("pendingssid", currentSettings.wifiSsid);
        currentSettings.wifiPassword = preferences.getString("pendingpass", currentSettings.wifiPassword);
        currentSettings.hostname = preferences.getString("pendinghost", currentSettings.hostname);
        Serial.println("[SettingsService] Pending WiFi candidate loaded for boot test");
    }


    // --------------------------------------------------------
    // Display
    // --------------------------------------------------------

    currentSettings.displayBrightness =
        preferences.getUChar(
            "brightness",
            currentSettings.displayBrightness);

    currentSettings.defaultScreen =
        preferences.getUChar(
            "defaultpage",
            currentSettings.defaultScreen);
    if (currentSettings.defaultScreen > 6)
    {
        currentSettings.defaultScreen = 0;
    }


    // --------------------------------------------------------
    // Weather
    // --------------------------------------------------------

    currentSettings.weatherRefreshMinutes =
        preferences.getUInt(
            "wxminutes",
            currentSettings.weatherRefreshMinutes);

    currentSettings.useFahrenheit =
        preferences.getBool(
            "fahrenheit",
            currentSettings.useFahrenheit);
}

void SettingsService::saveToStorage()
{
    preferences.putUShort(
        "schema",
        CURRENT_SCHEMA_VERSION);


    // --------------------------------------------------------
    // Station
    // --------------------------------------------------------

    preferences.putString(
        "callsign",
        currentSettings.callsign);

    preferences.putString(
        "grid",
        currentSettings.gridSquare);

    preferences.putString(
        "station",
        currentSettings.stationName);


    // --------------------------------------------------------
    // Location
    // --------------------------------------------------------

    preferences.putString(
        "location",
        currentSettings.locationName);

    preferences.putString(
        "postal",
        currentSettings.postalCode);

    preferences.putDouble(
        "latitude",
        currentSettings.latitude);

    preferences.putDouble(
        "longitude",
        currentSettings.longitude);


    // --------------------------------------------------------
    // Time
    // --------------------------------------------------------

    preferences.putString(
        "timezone",
        currentSettings.timezone);

    preferences.putBool(
        "use24hour",
        currentSettings.use24HourTime);


    // --------------------------------------------------------
    // Network
    // --------------------------------------------------------

    preferences.putString(
        "ssid",
        currentSettings.wifiSsid);

    preferences.putString(
        "wifipass",
        currentSettings.wifiPassword);

    preferences.putString(
        "hostname",
        currentSettings.hostname);


    // --------------------------------------------------------
    // Display
    // --------------------------------------------------------

    preferences.putUChar(
        "brightness",
        currentSettings.displayBrightness);

    preferences.putUChar(
        "defaultpage",
        currentSettings.defaultScreen);


    // --------------------------------------------------------
    // Weather
    // --------------------------------------------------------

    preferences.putUInt(
        "wxminutes",
        currentSettings.weatherRefreshMinutes);

    preferences.putBool(
        "fahrenheit",
        currentSettings.useFahrenheit);
}
