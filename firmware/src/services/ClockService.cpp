#include "ClockService.h"

#include "../config/Settings.h"

#include <WiFi.h>
#include <stdlib.h>
#include <time.h>

namespace
{
    // Cloudflare's numeric NTP addresses avoid a DNS lookup during startup.
    // These are the same endpoints used by the isolated network diagnostic.
    constexpr const char* PRIMARY_NTP = "162.159.200.1";
    constexpr const char* BACKUP_NTP = "162.159.200.123";
}

void ClockService::begin()
{
    Serial.println("Clock service started.");
    Serial.println("NTP will start after WiFi receives an IP address.");
}

void ClockService::update()
{
    if (!ntpConfigured && WiFi.status() == WL_CONNECTED)
    {
        // Use numeric NTP servers so time synchronization cannot contend for
        // DNS with panel services. configTime() sets its own UTC timezone, so
        // apply the configured local timezone after calling it.
        configTime(0, 0, PRIMARY_NTP, BACKUP_NTP);
        setenv("TZ", TIMEZONE, 1);
        tzset();
        ntpConfigured = true;
        Serial.println("Waiting for network time synchronization...");
    }

    if (ntpConfigured && !synchronizationReported && time(nullptr) >= MINIMUM_VALID_TIME)
    {
        synchronizationReported = true;
        Serial.println("Network time synchronized.");
    }
}

bool ClockService::isSynchronized()
{
    // time(nullptr) is immediate. getLocalTime() can wait for its timeout and
    // this method is called on every UI loop pass.
    return time(nullptr) >= MINIMUM_VALID_TIME;
}

bool ClockService::getLocalTimeInfo(struct tm& timeInfo)
{
    return ::getLocalTime(
        &timeInfo,
        10);
}

bool ClockService::getUTCTimeInfo(struct tm& timeInfo)
{
    time_t now;

    time(&now);

    /*
     * An unsynchronized ESP32 normally reports a time close to
     * January 1, 1970. Reject anything before 2024.
     */
    if (now < MINIMUM_VALID_TIME)
    {
        return false;
    }

    gmtime_r(
        &now,
        &timeInfo);

    return true;
}

String ClockService::getLocalTime()
{
    struct tm timeInfo;

    if (!getLocalTimeInfo(timeInfo))
    {
        return "--:--:--";
    }

    char buffer[12];

    strftime(
        buffer,
        sizeof(buffer),
        "%H:%M:%S",
        &timeInfo);

    return String(buffer);
}

String ClockService::getUTCTime()
{
    struct tm timeInfo;

    if (!getUTCTimeInfo(timeInfo))
    {
        return "--:--:--";
    }

    char buffer[12];

    strftime(
        buffer,
        sizeof(buffer),
        "%H:%M:%S",
        &timeInfo);

    return String(buffer);
}

String ClockService::getLocalDate()
{
    struct tm timeInfo;

    if (!getLocalTimeInfo(timeInfo))
    {
        return "Waiting for time...";
    }

    char buffer[32];

    strftime(
        buffer,
        sizeof(buffer),
        "%A, %B %d",
        &timeInfo);

    return String(buffer);
}

String ClockService::getDisplayDate()
{
    struct tm timeInfo;

    if (!getLocalTimeInfo(timeInfo))
    {
        return "Waiting for time...";
    }

    char buffer[24];
    strftime(buffer, sizeof(buffer), "%B %d, %Y", &timeInfo);
    return String(buffer);
}

String ClockService::getUTCDate()
{
    struct tm timeInfo;

    if (!getUTCTimeInfo(timeInfo))
    {
        return "Waiting for time...";
    }

    char buffer[20];

    strftime(
        buffer,
        sizeof(buffer),
        "%d %b %Y",
        &timeInfo);

    return String(buffer);
}
