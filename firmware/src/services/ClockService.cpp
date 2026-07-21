#include "ClockService.h"
#include "../config/Settings.h"
#include <time.h>


void ClockService::begin()
{
    configTzTime(
        TIMEZONE,
        "pool.ntp.org",
        "time.nist.gov"
    );

    Serial.println("Waiting for time sync...");

    struct tm timeinfo;

    while (!::getLocalTime(&timeinfo))
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("Time synchronized");
}


String ClockService::formatTime()
{
    struct tm timeinfo;

    if (!::getLocalTime(&timeinfo))
    {
        return "--:--:--";
    }

    char buffer[10];

    sprintf(
        buffer,
        "%02d%02d%02d",
        timeinfo.tm_hour,
        timeinfo.tm_min,
        timeinfo.tm_sec
    );

    return String(buffer);
}


String ClockService::getLocalTime()
{
    return formatTime();
}


String ClockService::getUTCTime()
{
    time_t now;

    time(&now);

    struct tm utcTime;

    gmtime_r(&now, &utcTime);

    char buffer[10];

    sprintf(
        buffer,
        "%02d%02d%02d",
        utcTime.tm_hour,
        utcTime.tm_min,
        utcTime.tm_sec
    );

    return String(buffer);
}