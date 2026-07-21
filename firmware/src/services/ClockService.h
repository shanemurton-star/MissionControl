#pragma once

#include <Arduino.h>

class ClockService
{
public:
    void begin();

    String getLocalTime();
    String getUTCTime();

private:
    String formatTime();
};