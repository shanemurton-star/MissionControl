#pragma once

#include <Arduino.h>

class DisplayService
{
public:
    bool begin();
    void update();

private:
    uint32_t lastTickMillis = 0;
};