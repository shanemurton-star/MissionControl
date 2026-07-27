#pragma once

// ============================================================
// Mission Control default settings
//
// These values are used:
//   1. The first time the device starts
//   2. After a factory reset
//
// Normal user changes will eventually be stored in ESP32 NVS
// and edited through the touchscreen Settings screen.
// ============================================================


// ------------------------------------------------------------
// Station defaults
// ------------------------------------------------------------

#define DEFAULT_CALLSIGN ""

#define DEFAULT_GRID_SQUARE "EN82"

#define DEFAULT_STATION_NAME "Home Station"


// ------------------------------------------------------------
// Location defaults
// ------------------------------------------------------------

#define LOCATION_NAME "Holly, MI"

#define DEFAULT_LATITUDE 42.7900

#define DEFAULT_LONGITUDE -83.6300


// ------------------------------------------------------------
// Time defaults
// Eastern Time with automatic daylight-saving adjustment
// ------------------------------------------------------------

#define TIMEZONE "EST5EDT,M3.2.0,M11.1.0"

#define DEFAULT_USE_24_HOUR_TIME true


// ------------------------------------------------------------
// Network defaults
// ------------------------------------------------------------

#define DEFAULT_HOSTNAME "mission-control"


// ------------------------------------------------------------
// Weather defaults
// ------------------------------------------------------------

#define DEFAULT_WEATHER_REFRESH_MINUTES 10

#define DEFAULT_USE_FAHRENHEIT true


// ------------------------------------------------------------
// Feature defaults
// ------------------------------------------------------------

#define ENABLE_WEATHER true

#define ENABLE_RADIO true

#define ENABLE_SATELLITES true

#define ENABLE_CALENDAR true


// ------------------------------------------------------------
// Display defaults
// ------------------------------------------------------------

#define DISPLAY_BRIGHTNESS 80