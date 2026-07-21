#include "StatusService.h"


void StatusService::begin()
{
    systemStatus = "NOMINAL";

    Serial.println("Status Service Started");
}


void StatusService::setStatus(
    String name,
    String value
)
{
    if (name == "WIFI")
    {
        wifiStatus = value;
    }
    else if (name == "TIME")
    {
        timeStatus = value;
    }
    else if (name == "WEATHER")
    {
        weatherStatus = value;
    }
    else if (name == "SYSTEM")
    {
        systemStatus = value;
    }
}


String StatusService::getStatus(
    String name
)
{
    if (name == "WIFI")
    {
        return wifiStatus;
    }


    if (name == "TIME")
    {
        return timeStatus;
    }


    if (name == "WEATHER")
    {
        return weatherStatus;
    }


    if (name == "SYSTEM")
    {
        return systemStatus;
    }


    return "UNKNOWN";
}


void StatusService::printStatus()
{
    Serial.println();
    Serial.println("----------------------------");
    Serial.println("MISSION STATUS");


    Serial.print("WIFI:    ");
    Serial.println(wifiStatus);


    Serial.print("TIME:    ");
    Serial.println(timeStatus);


    Serial.print("WEATHER: ");
    Serial.println(weatherStatus);


    Serial.print("SYSTEM:  ");
    Serial.println(systemStatus);


    Serial.println("----------------------------");
}