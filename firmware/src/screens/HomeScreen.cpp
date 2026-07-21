#include "HomeScreen.h"


void HomeScreen::draw(
    ClockService& clock,
    WiFiService& wifi
)
{
    Serial.println();
    Serial.println("============================");
    Serial.println("       MISSION CONTROL");
    Serial.println("============================");

    Serial.print("LOCAL: ");
    Serial.println(clock.getLocalTime());

    Serial.print("UTC:   ");
    Serial.println(clock.getUTCTime());

    Serial.print("WIFI:  ");

    if(wifi.isConnected())
    {
        Serial.println("ONLINE");
    }
    else
    {
        Serial.println("OFFLINE");
    }

    Serial.println("SYSTEM NOMINAL");
    Serial.println("============================");
}