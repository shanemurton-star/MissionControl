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


    Serial.print("LOCAL:  ");
    Serial.println(clock.getLocalTime());


    Serial.print("UTC:    ");
    Serial.println(clock.getUTCTime());


    Serial.println();

    Serial.println("SYSTEM STATUS");


    Serial.print("WIFI:   ");
    Serial.println(
        status.getStatus("WIFI")
    );


    Serial.print("TIME:   ");
    Serial.println(
        status.getStatus("TIME")
    );


    Serial.print("SYSTEM: ");
    Serial.println(
        status.getStatus("SYSTEM")
    );


    Serial.println("============================");
}