#include "ClockService.h"

#include "../config/Settings.h"

#include <time.h>

void ClockService::begin()
{
    configTzTime(
        TIMEZONE,
        "pool.ntp.org",
        "time.nist.gov");

    Serial.println("Clock service started.");
    Serial.println("Waiting for network time synchronization...");
}

bool ClockService::isSynchronized()
{
    struct tm timeInfo;
    return getLocalTimeInfo(timeInfo);
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
    constexpr time_t MINIMUM_VALID_TIME = 1704067200;

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
