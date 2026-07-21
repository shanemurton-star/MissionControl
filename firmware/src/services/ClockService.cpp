#include "ClockService.h"
#include <time.h>

void ClockService::begin()
{
    configTime(
        0,
        0,
        "pool.ntp.org",
        "time.nist.gov"
    );
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


String ClockService::getUTCTime()
{
    return formatTime();
}


String ClockService::getLocalTime()
{
    return formatTime();
}