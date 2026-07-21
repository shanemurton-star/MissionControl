#pragma once

#include <Arduino.h>


enum ScreenType
{
    HOME,
    WEATHER,
    RADIO,
    SPACE,
    CALENDAR
};


class ScreenManager
{
public:

    void begin();

    void show(ScreenType screen);

    void update();

private:

    ScreenType currentScreen = HOME;
};