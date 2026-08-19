#pragma once

#include <functional>

#include <lvgl.h>

#include "../models/Page.h"
#include "../services/ClockService.h"
#include "../services/AircraftService.h"
#include "../services/SatelliteService.h"
#include "../services/SolarService.h"
#include "../services/LiveSpotsService.h"
#include "../services/PotaService.h"
#include "../services/WeatherService.h"
#include "../ui/HeaderBar.h"
#include "../ui/AircraftPanel.h"
#include "../ui/SatellitePanel.h"
#include "../ui/SolarPanel.h"
#include "../ui/LiveSpotsPanel.h"
#include "../ui/WeatherPanel.h"
#include "../ui/PotaPanel.h"

class DashboardScreen
{
public:
    using NavigationCallback =
        std::function<void(Page)>;

    void begin(
        ClockService& clockService,
        WeatherService& weatherService,
        AircraftService& aircraftService,
        SatelliteService& satelliteService,
        SolarService& solarService,
        LiveSpotsService& liveSpotsService,
        PotaService& potaService);

    void show();

    void setNavigationCallback(
        NavigationCallback callback);

private:
    static void updateTimerCallback(
        lv_timer_t* timer);

    void update();

    lv_obj_t* screen = nullptr;

    HeaderBar headerBar;

    // Dashboard panels
    WeatherPanel weatherPanel;
    AircraftPanel aircraftPanel;
    SatellitePanel satellitePanel;
    SolarPanel solarPanel;
    LiveSpotsPanel liveSpotsPanel;
    PotaPanel potaPanel;

    // Navigation from dashboard panels to detail screens
    NavigationCallback navigationCallback;

    lv_timer_t* updateTimer = nullptr;

    ClockService* clockService = nullptr;
    WeatherService* weatherService = nullptr;
    AircraftService* aircraftService = nullptr;
    SatelliteService* satelliteService = nullptr;
};
