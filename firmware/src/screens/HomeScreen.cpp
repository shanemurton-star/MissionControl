#include "HomeScreen.h"


void HomeScreen::draw(
    ClockService& clock,
    StatusService& status
)
{
    Serial.println();

    Serial.println("============================");
    Serial.println("       MISSION CONTROL");
    Serial.println("============================");


    Serial.println();
    Serial.println("TIME");

    Serial.print("LOCAL: ");
    Serial.println(
        clock.getLocalTime()
    );


    Serial.print("UTC:   ");
    Serial.println(
        clock.getUTCTime()
    );


    Serial.println();
    Serial.println("SYSTEMS");


    Serial.print("WIFI:    ");
    Serial.println(
        status.getStatus("WIFI")
    );


    Serial.print("TIME:    ");
    Serial.println(
        status.getStatus("TIME")
    );


    Serial.println();
    Serial.println("ENVIRONMENT");


    Serial.print("WEATHER: ");
    Serial.println(
        status.getStatus("WEATHER")
    );


    Serial.println();
    Serial.println("STATUS");


    Serial.print("SYSTEM:  ");
    Serial.println(
        status.getStatus("SYSTEM")
    );


    Serial.println();
    Serial.println("============================");
}