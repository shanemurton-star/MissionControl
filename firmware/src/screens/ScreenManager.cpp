#include "ScreenManager.h"


void ScreenManager::begin()
{
    Serial.println("Screen Manager Started");
}


void ScreenManager::show(ScreenType screen)
{
    currentScreen = screen;

    Serial.print("Switching to screen: ");

    switch(screen)
    {
        case HOME:
            Serial.println("HOME");
            break;

        case WEATHER:
            Serial.println("WEATHER");
            break;

        case RADIO:
            Serial.println("RADIO");
            break;

        case SPACE:
            Serial.println("SPACE");
            break;

        case CALENDAR:
            Serial.println("CALENDAR");
            break;
    }
}


void ScreenManager::update()
{
    // Later this will call LVGL rendering
}