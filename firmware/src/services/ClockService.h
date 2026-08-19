#pragma once

#include <Arduino.h>

class ClockService
{
public:
    void begin();

    bool isSynchronized();

    String getLocalTime();
    String getUTCTime();

    String getLocalDate();
    String getDisplayDate();
    String getUTCDate();

private:
    bool getLocalTimeInfo(struct tm& timeInfo);
    bool getUTCTimeInfo(struct tm& timeInfo);
};
