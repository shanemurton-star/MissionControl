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
    bool getLocalTimeInfo(struct tm& timeInfo);
    bool getUTCTimeInfo(struct tm& timeInfo);
    bool ntpConfigured = false;
    bool synchronizationReported = false;
};
