#pragma once

#include <Arduino.h>

class ClockService
{
public:
    void begin();
    void update();

    bool isSynchronized();

    String getLocalTime();
    String getUTCTime();

    String getLocalDate();
    String getDisplayDate();
    String getUTCDate();

private:
    static constexpr time_t MINIMUM_VALID_TIME = 1704067200;
    bool getLocalTimeInfo(struct tm& timeInfo);
    bool getUTCTimeInfo(struct tm& timeInfo);
    bool ntpConfigured = false;
    bool synchronizationReported = false;
};
