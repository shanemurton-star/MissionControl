#pragma once

#include <Arduino.h>


class StatusService
{
public:

    void begin();

    void setStatus(
        String name,
        String value
    );

    String getStatus(
        String name
    );

    void printStatus();


private:

    String wifiStatus = "UNKNOWN";
    String timeStatus = "UNKNOWN";
    String systemStatus = "STARTING";
};