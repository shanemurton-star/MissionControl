#pragma once

#include <Arduino.h>

#include "../services/ClockService.h"
#include "../services/StatusService.h"


class HomeScreen
{
public:

    void draw(
        ClockService& clock,
        StatusService& status
    );
};