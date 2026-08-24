#pragma once

#include <Arduino.h>
#include <lvgl.h>

namespace AircraftArtwork
{
    enum class Family : uint8_t
    {
        NarrowBody,
        LongNarrowBody,
        WideBody,
        RegionalJet,
        BusinessJet,
        Turboprop,
        LightAircraft,
        Helicopter,
        HighPerformance,
        Boeing747,
        AirbusA380,
        Boeing777,
        Boeing787,
        AirbusA350,
        Crj,
        EmbraerEJet,
        Atr,
        Dash8,
        Gulfstream,
        Citation,
        Cessna,
        PiperSuperCub,
        PiperCherokee,
        PiperSeneca,
        PiperMClass,
        Dhc2Beaver,
        Dhc6TwinOtter,
        BeechBonanza,
        BeechVtailBonanza,
        BeechBaron,
        BeechKingAir
    };

    Family classify(
        const String& type, const String& description, const String& category);
    const lv_img_dsc_t* imageFor(Family family);
    const char* labelFor(Family family);
}
