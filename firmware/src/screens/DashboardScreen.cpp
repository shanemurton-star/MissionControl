#include "DashboardScreen.h"
#include "../models/Page.h"
#include "../ui/NavigationBar.h"

#include <WiFi.h>

namespace
{
    constexpr int16_t SCREEN_WIDTH = 800;
    constexpr int16_t SCREEN_HEIGHT = 480;

    constexpr int16_t HEADER_HEIGHT = 58;
    constexpr int16_t FOOTER_HEIGHT = 46;

    constexpr int16_t PANEL_GAP = 8;
    constexpr int16_t PANEL_TOP = 66;
    constexpr int16_t PANEL_HEIGHT = 172;

    constexpr int16_t LEFT_PANEL_X = 8;
    constexpr int16_t CENTER_PANEL_X = 268;
    constexpr int16_t RIGHT_PANEL_X = 528;

    constexpr int16_t LEFT_PANEL_WIDTH = 252;
    constexpr int16_t CENTER_PANEL_WIDTH = 252;
    constexpr int16_t RIGHT_PANEL_WIDTH = 264;

    lv_color_t colorFromHex(uint32_t value)
    {
        return lv_color_hex(value);
    }

    void configureScreen(lv_obj_t* object)
    {
        lv_obj_set_style_bg_color(
            object,
            colorFromHex(0x06111D),
            LV_PART_MAIN);

        lv_obj_set_style_bg_opa(
            object,
            LV_OPA_COVER,
            LV_PART_MAIN);

        lv_obj_clear_flag(
            object,
            LV_OBJ_FLAG_SCROLLABLE);
    }

    void configurePanel(lv_obj_t* panel)
    {
        lv_obj_set_style_bg_color(
            panel,
            colorFromHex(0x0D1B29),
            LV_PART_MAIN);

        lv_obj_set_style_bg_opa(
            panel,
            LV_OPA_COVER,
            LV_PART_MAIN);

        lv_obj_set_style_border_color(
            panel,
            colorFromHex(0x1D7891),
            LV_PART_MAIN);

        lv_obj_set_style_border_width(
            panel,
            1,
            LV_PART_MAIN);

        lv_obj_set_style_radius(
            panel,
            8,
            LV_PART_MAIN);

        lv_obj_set_style_pad_all(
            panel,
            10,
            LV_PART_MAIN);

        lv_obj_clear_flag(
            panel,
            LV_OBJ_FLAG_SCROLLABLE);
    }

    void configureHeaderBar(lv_obj_t* header)
    {
        lv_obj_set_style_bg_color(
            header,
            colorFromHex(0x0A1825),
            LV_PART_MAIN);

        lv_obj_set_style_bg_opa(
            header,
            LV_OPA_COVER,
            LV_PART_MAIN);

        lv_obj_set_style_border_width(
            header,
            0,
            LV_PART_MAIN);

        lv_obj_set_style_radius(
            header,
            0,
            LV_PART_MAIN);

        lv_obj_set_style_pad_all(
            header,
            0,
            LV_PART_MAIN);

        lv_obj_clear_flag(
            header,
            LV_OBJ_FLAG_SCROLLABLE);
    }

    lv_obj_t* createLabel(
        lv_obj_t* parent,
        const char* text,
        uint32_t color)
    {
        lv_obj_t* label = lv_label_create(parent);

        lv_label_set_text(
            label,
            text);

        lv_obj_set_style_text_color(
            label,
            colorFromHex(color),
            LV_PART_MAIN);

        lv_obj_set_style_text_font(
            label,
            &lv_font_montserrat_14,
            LV_PART_MAIN);

        return label;
    }

    lv_obj_t* createPanel(
        lv_obj_t* parent,
        int16_t x,
        int16_t y,
        int16_t width,
        int16_t height,
        const char* title)
    {
        lv_obj_t* panel = lv_obj_create(parent);

        lv_obj_set_pos(
            panel,
            x,
            y);

        lv_obj_set_size(
            panel,
            width,
            height);

        configurePanel(panel);

        lv_obj_t* titleLabel = createLabel(
            panel,
            title,
            0x32C7E8);

        lv_obj_align(
            titleLabel,
            LV_ALIGN_TOP_LEFT,
            0,
            0);

        return panel;
    }

    void createSectionDivider(
        lv_obj_t* parent,
        int16_t y,
        int16_t width)
    {
        lv_obj_t* divider = lv_obj_create(parent);

        lv_obj_set_pos(
            divider,
            0,
            y);

        lv_obj_set_size(
            divider,
            width,
            1);

        lv_obj_set_style_bg_color(
            divider,
            colorFromHex(0x1D7891),
            LV_PART_MAIN);

        lv_obj_set_style_bg_opa(
            divider,
            LV_OPA_COVER,
            LV_PART_MAIN);

        lv_obj_set_style_border_width(
            divider,
            0,
            LV_PART_MAIN);

        lv_obj_clear_flag(
            divider,
            LV_OBJ_FLAG_SCROLLABLE);
    }
}

void DashboardScreen::begin(
    ClockService& clockServiceReference,
    WeatherService& weatherServiceReference)
{
    if (screen != nullptr)
    {
        return;
    }

    clockService = &clockServiceReference;
    weatherService = &weatherServiceReference;

    screen = lv_obj_create(nullptr);

    lv_obj_set_size(
        screen,
        SCREEN_WIDTH,
        SCREEN_HEIGHT);

    configureScreen(screen);

    /*
     * Header
     */
    lv_obj_t* header = lv_obj_create(screen);

    lv_obj_set_pos(
        header,
        0,
        0);

    lv_obj_set_size(
        header,
        SCREEN_WIDTH,
        HEADER_HEIGHT);

    configureHeaderBar(header);

    lv_obj_t* titleLabel = createLabel(
        header,
        "MISSION CONTROL",
        0xFFFFFF);

    lv_obj_align(
        titleLabel,
        LV_ALIGN_TOP_LEFT,
        12,
        8);

    lv_obj_t* stationLabel = createLabel(
        header,
        "HOME STATION - EN82",
        0x32C7E8);

    lv_obj_align(
        stationLabel,
        LV_ALIGN_BOTTOM_LEFT,
        12,
        -7);

    lv_obj_t* localCaption = createLabel(
        header,
        "LOCAL",
        0x7893A8);

    lv_obj_align(
        localCaption,
        LV_ALIGN_TOP_MID,
        -135,
        5);

    localTimeLabel = createLabel(
        header,
        "--:--:--",
        0xFFFFFF);

    lv_obj_align(
        localTimeLabel,
        LV_ALIGN_BOTTOM_MID,
        -135,
        -6);

    lv_obj_t* utcCaption = createLabel(
        header,
        "UTC",
        0x7893A8);

    lv_obj_align(
        utcCaption,
        LV_ALIGN_TOP_MID,
        -25,
        5);

    utcTimeLabel = createLabel(
        header,
        "--:--:--",
        0xFFFFFF);

    lv_obj_align(
        utcTimeLabel,
        LV_ALIGN_BOTTOM_MID,
        -25,
        -6);

    wifiStatusLabel = createLabel(
        header,
        "WIFI --",
        0xF4C95D);

    lv_obj_align(
        wifiStatusLabel,
        LV_ALIGN_TOP_RIGHT,
        -118,
        8);

    ntpStatusLabel = createLabel(
        header,
        "NTP --",
        0xF4C95D);

    lv_obj_align(
        ntpStatusLabel,
        LV_ALIGN_BOTTOM_RIGHT,
        -118,
        -7);

    lv_obj_t* insideTempLabel = createLabel(
        header,
        "INSIDE  -- F",
        0x8EA9C1);

    lv_obj_align(
        insideTempLabel,
        LV_ALIGN_TOP_RIGHT,
        -10,
        8);

    lv_obj_t* powerLabel = createLabel(
        header,
        "POWER  --.-V",
        0x8EA9C1);

    lv_obj_align(
        powerLabel,
        LV_ALIGN_BOTTOM_RIGHT,
        -10,
        -7);

    createSectionDivider(
        screen,
        HEADER_HEIGHT,
        SCREEN_WIDTH);

    /*
     * Top-left: Weather
     */
    lv_obj_t* weatherPanel = createPanel(
        screen,
        LEFT_PANEL_X,
        PANEL_TOP,
        LEFT_PANEL_WIDTH,
        PANEL_HEIGHT,
        "WEATHER");

    weatherConditionLabel = createLabel(
        weatherPanel,
        "WAITING FOR WEATHER",
        0xFFFFFF);

    lv_obj_align(
        weatherConditionLabel,
        LV_ALIGN_TOP_LEFT,
        0,
        32);

    weatherTempLabel = createLabel(
        weatherPanel,
        "-- F",
        0xFFFFFF);

    lv_obj_align(
        weatherTempLabel,
        LV_ALIGN_TOP_RIGHT,
        0,
        32);

    weatherDetailsLabel = createLabel(
        weatherPanel,
        "Humidity     --%\n"
        "Wind         -- mph\n"
        "Pressure     --.-- inHg\n"
        "Station      --",
        0x8EA9C1);

    lv_obj_align(
        weatherDetailsLabel,
        LV_ALIGN_BOTTOM_LEFT,
        0,
        -2);

    /*
     * Top-center: Radio
     */
    lv_obj_t* radioPanel = createPanel(
        screen,
        CENTER_PANEL_X,
        PANEL_TOP,
        CENTER_PANEL_WIDTH,
        PANEL_HEIGHT,
        "IC-7300 RADIO");

    lv_obj_t* frequencyLabel = createLabel(
        radioPanel,
        "14.074.000",
        0xFFFFFF);

    lv_obj_align(
        frequencyLabel,
        LV_ALIGN_TOP_MID,
        0,
        31);

    lv_obj_t* radioModeLabel = createLabel(
        radioPanel,
        "USB-D          20m",
        0x32C7E8);

    lv_obj_align(
        radioModeLabel,
        LV_ALIGN_TOP_MID,
        0,
        58);

    lv_obj_t* radioDetails = createLabel(
        radioPanel,
        "S-Meter     S0\n"
        "SWR         1.0\n"
        "ALC         0\n"
        "Power       0 W",
        0x8EA9C1);

    lv_obj_align(
        radioDetails,
        LV_ALIGN_BOTTOM_LEFT,
        0,
        -2);

    /*
     * Top-right: Satellite
     */
    lv_obj_t* satellitePanel = createPanel(
        screen,
        RIGHT_PANEL_X,
        PANEL_TOP,
        RIGHT_PANEL_WIDTH,
        PANEL_HEIGHT,
        "NEXT SATELLITE PASS");

    lv_obj_t* satelliteName = createLabel(
        satellitePanel,
        "ISS (ZARYA)",
        0xFFFFFF);

    lv_obj_align(
        satelliteName,
        LV_ALIGN_TOP_LEFT,
        0,
        32);

    lv_obj_t* satelliteCountdown = createLabel(
        satellitePanel,
        "T- --:--:--",
        0x57D68D);

    lv_obj_align(
        satelliteCountdown,
        LV_ALIGN_TOP_RIGHT,
        0,
        32);

    lv_obj_t* satelliteDetails = createLabel(
        satellitePanel,
        "AOS          --:--\n"
        "Max Elev     -- deg\n"
        "Direction    ---\n"
        "Duration     -- min",
        0x8EA9C1);

    lv_obj_align(
        satelliteDetails,
        LV_ALIGN_BOTTOM_LEFT,
        0,
        -2);

    /*
     * Bottom-left: Aircraft
     */
    lv_obj_t* aircraftPanel = createPanel(
        screen,
        LEFT_PANEL_X,
        PANEL_TOP + PANEL_HEIGHT + PANEL_GAP,
        LEFT_PANEL_WIDTH,
        PANEL_HEIGHT,
        "AIRCRAFT NEARBY");

    lv_obj_t* aircraftCount = createLabel(
        aircraftPanel,
        "-- TRACKED",
        0xFFFFFF);

    lv_obj_align(
        aircraftCount,
        LV_ALIGN_TOP_LEFT,
        0,
        32);

    lv_obj_t* radarPlaceholder = createLabel(
        aircraftPanel,
        "       .       \n"
        "   .   +   .   \n"
        "       |       \n"
        " . ----+---- . \n"
        "       |       \n"
        "   .   +   .   ",
        0x1D7891);

    lv_obj_align(
        radarPlaceholder,
        LV_ALIGN_BOTTOM_MID,
        0,
        -1);

    /*
     * Bottom-center: Solar
     */
    lv_obj_t* solarPanel = createPanel(
        screen,
        CENTER_PANEL_X,
        PANEL_TOP + PANEL_HEIGHT + PANEL_GAP,
        CENTER_PANEL_WIDTH,
        PANEL_HEIGHT,
        "SOLAR CONDITIONS");

    lv_obj_t* solarDetails = createLabel(
        solarPanel,
        "Solar Flux       ---\n"
        "K-Index          --\n"
        "A-Index          --\n"
        "X-Ray Flux       ----\n"
        "Propagation      UNKNOWN",
        0x8EA9C1);

    lv_obj_align(
        solarDetails,
        LV_ALIGN_TOP_LEFT,
        0,
        34);

    /*
     * Bottom-right: Events
     */
    lv_obj_t* eventsPanel = createPanel(
        screen,
        RIGHT_PANEL_X,
        PANEL_TOP + PANEL_HEIGHT + PANEL_GAP,
        RIGHT_PANEL_WIDTH,
        PANEL_HEIGHT,
        "UPCOMING EVENTS");

    lv_obj_t* eventsDetails = createLabel(
        eventsPanel,
        "--:--   No events loaded\n\n"
        "--:--   Calendar offline\n\n"
        "--:--   Waiting for sync",
        0x8EA9C1);

    lv_obj_align(
        eventsDetails,
        LV_ALIGN_TOP_LEFT,
        0,
        34);

    /*
     * Footer status bar
     */
    /*
 * Bottom navigation
 */
navigationBar.create(screen);
navigationBar.setSelected(Page::Dashboard);

    update();

    updateTimer = lv_timer_create(
        updateTimerCallback,
        1000,
        this);
}

void DashboardScreen::show()
{
    if (screen == nullptr)
    {
        return;
    }

    lv_scr_load(screen);
    update();
}

void DashboardScreen::update()
{
    /*
     * Allow the weather service to advance through its states:
     * waiting for Wi-Fi, resolving the station and downloading data.
     */
    weatherService->update();

    if (localTimeLabel == nullptr ||
    utcTimeLabel == nullptr ||
    wifiStatusLabel == nullptr ||
    ntpStatusLabel == nullptr ||
    weatherConditionLabel == nullptr ||
    weatherTempLabel == nullptr ||
    weatherDetailsLabel == nullptr)
    {
        return;
    }

    const String localTime =
        clockService->getLocalTime();

    const String utcTime =
        clockService->getUTCTime();

    lv_label_set_text(
        localTimeLabel,
        localTime.c_str());

    lv_label_set_text(
        utcTimeLabel,
        utcTime.c_str());

    const bool wifiConnected =
        WiFi.status() == WL_CONNECTED;

    const bool timeSynchronized =
        clockService->isSynchronized();

    /*
     * Wi-Fi status
     */
    if (wifiConnected)
    {
        lv_label_set_text(
            wifiStatusLabel,
            "WIFI OK");

        lv_obj_set_style_text_color(
            wifiStatusLabel,
            colorFromHex(0x57D68D),
            LV_PART_MAIN);
    }
    else
    {
        lv_label_set_text(
            wifiStatusLabel,
            "WIFI --");

        lv_obj_set_style_text_color(
            wifiStatusLabel,
            colorFromHex(0xF4C95D),
            LV_PART_MAIN);
    }

    /*
     * NTP status
     */
    if (timeSynchronized)
    {
        lv_label_set_text(
            ntpStatusLabel,
            "NTP OK");

        lv_obj_set_style_text_color(
            ntpStatusLabel,
            colorFromHex(0x57D68D),
            LV_PART_MAIN);
    }
    else
    {
        lv_label_set_text(
            ntpStatusLabel,
            "NTP --");

        lv_obj_set_style_text_color(
            ntpStatusLabel,
            colorFromHex(0xF4C95D),
            LV_PART_MAIN);
    }

    /*
     * Weather panel
     */
    if (!wifiConnected)
    {
        lv_label_set_text(
            weatherConditionLabel,
            "WAITING FOR WIFI");

        lv_label_set_text(
            weatherTempLabel,
            "-- F");

        lv_label_set_text(
            weatherDetailsLabel,
            "Humidity     --%\n"
            "Wind         -- mph\n"
            "Pressure     --.-- inHg\n"
            "Station      --");

        lv_obj_set_style_text_color(
            weatherConditionLabel,
            colorFromHex(0xF4C95D),
            LV_PART_MAIN);
    }
    else if (weatherService->isValid())
    {
        const WeatherData& weather =
            weatherService->getCurrentWeather();

        const String temperature =
            weatherService->getTemperature();

        const String humidity =
            weatherService->getHumidity();

        const String wind =
            weatherService->getWind();

        const String pressure =
            weatherService->getPressure();

        const String station =
            weatherService->getStationId();

        lv_label_set_text(
            weatherConditionLabel,
            weather.condition.c_str());

        lv_label_set_text(
            weatherTempLabel,
            temperature.c_str());

        String weatherDetails;

        weatherDetails.reserve(128);

        weatherDetails += "Humidity     ";
        weatherDetails += humidity;
        weatherDetails += "\n";

        weatherDetails += "Wind         ";
        weatherDetails += wind;
        weatherDetails += "\n";

        weatherDetails += "Pressure     ";
        weatherDetails += pressure;
        weatherDetails += "\n";

        weatherDetails += "Station      ";
        weatherDetails += station;

        lv_label_set_text(
            weatherDetailsLabel,
            weatherDetails.c_str());

        lv_obj_set_style_text_color(
            weatherConditionLabel,
            colorFromHex(0xFFFFFF),
            LV_PART_MAIN);
    }
    else if (weatherService->isUpdating())
    {
        lv_label_set_text(
            weatherConditionLabel,
            "UPDATING WEATHER");

        lv_label_set_text(
            weatherTempLabel,
            "-- F");

        lv_label_set_text(
            weatherDetailsLabel,
            "Contacting National\n"
            "Weather Service...\n\n"
            "Please wait");

        lv_obj_set_style_text_color(
            weatherConditionLabel,
            colorFromHex(0xF4C95D),
            LV_PART_MAIN);
    }
    else
    {
        lv_label_set_text(
            weatherConditionLabel,
            "WEATHER UNAVAILABLE");

        lv_label_set_text(
            weatherTempLabel,
            "-- F");

        const String errorMessage =
            weatherService->getLastError();

        if (errorMessage.isEmpty())
        {
            lv_label_set_text(
                weatherDetailsLabel,
                "Waiting for weather\n"
                "service to update...");
        }
        else
        {
            lv_label_set_text(
                weatherDetailsLabel,
                errorMessage.c_str());
        }

        lv_obj_set_style_text_color(
            weatherConditionLabel,
            colorFromHex(0xF4C95D),
            LV_PART_MAIN);
    }

   
}

void DashboardScreen::updateTimerCallback(lv_timer_t* timer)
{
    DashboardScreen* dashboard =
        static_cast<DashboardScreen*>(
            timer->user_data);

    if (dashboard == nullptr)
    {
        return;
    }

    dashboard->update();
}

NavigationBar& DashboardScreen::getNavigationBar()
{
    return navigationBar;
}