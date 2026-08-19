#pragma once

#include <Arduino.h>

/**
 * All user-configurable Mission Control settings.
 *
 * This model contains the current in-memory values. SettingsService
 * is responsible for loading and saving the values in ESP32 NVS.
 */
struct AppSettings
{
    // --------------------------------------------------------
    // Station profile
    // --------------------------------------------------------

    String callsign;
    String gridSquare;
    String stationName;


    // --------------------------------------------------------
    // Geographic location
    // --------------------------------------------------------

    String locationName;
    String postalCode;

    double latitude = 0.0;
    double longitude = 0.0;


    // --------------------------------------------------------
    // Time
    // --------------------------------------------------------

    String timezone;
    bool use24HourTime = true;


    // --------------------------------------------------------
    // Network
    // --------------------------------------------------------

    String wifiSsid;
    String wifiPassword;
    String hostname;


    // --------------------------------------------------------
    // Display
    // --------------------------------------------------------

    uint8_t displayBrightness = 80;


    // --------------------------------------------------------
    // Weather
    // --------------------------------------------------------

    uint32_t weatherRefreshMinutes = 10;
    bool useFahrenheit = true;
};
