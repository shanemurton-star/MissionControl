#include "WeatherService.h"


void WeatherService::begin()
{
    Serial.println("Weather Service Started");

    temperature = "--";
    conditions = "WAITING";
    humidity = "--";
    wind = "--";
}


void WeatherService::update(
    StatusService& status
)
{
    // Temporary simulated weather data
    // NOAA API will replace this later

    temperature = "72F";
    conditions = "CLEAR";
    humidity = "45%";
    wind = "8 MPH";


    status.setStatus(
        "WEATHER",
        temperature + " " + conditions
    );
}


String WeatherService::getTemperature()
{
    return temperature;
}


String WeatherService::getConditions()
{
    return conditions;
}


String WeatherService::getHumidity()
{
    return humidity;
}


String WeatherService::getWind()
{
    return wind;
}