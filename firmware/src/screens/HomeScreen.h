#pragma once

#include <Arduino.h>
#include "../services/ClockService.h"
#include "../services/WiFiService.h"


class HomeScreen
{
public:

    void draw(
        ClockService& clock,
        WiFiService& wifi
    );
};