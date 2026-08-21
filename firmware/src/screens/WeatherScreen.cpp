#include "WeatherScreen.h"
#include "../services/NetworkUpdateState.h"

#include <math.h>
#include "../ui/DashboardIcons.h"
#include "../ui/Theme.h"

namespace
{
    String aqiCategory(int16_t aqi)
    {
        if (aqi <= 50) return "GOOD";
        if (aqi <= 100) return "MODERATE";
        if (aqi <= 150) return "UNHEALTHY FOR SENSITIVE";
        if (aqi <= 200) return "UNHEALTHY";
        if (aqi <= 300) return "VERY UNHEALTHY";
        return "HAZARDOUS";
    }

    String valueOrDash(float value, uint8_t decimals, const char* suffix)
    {
        return isnan(value) || isinf(value)
            ? String("--") + suffix
            : String(value, static_cast<unsigned int>(decimals)) + suffix;
    }
}

void WeatherScreen::begin(
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

    Theme::configureScreen(screen);

    headerBar.create(
        screen,
        clockServiceReference,
        Theme::SCREEN_WIDTH,
        Theme::HEADER_HEIGHT);
    headerBar.setSettingsCallback([this]() {
        if (navigationCallback != nullptr) navigationCallback(Page::Settings);
    });
    headerBar.setNavigationCallback([this](Page page) {
        if (navigationCallback != nullptr) navigationCallback(page);
    });

    lv_obj_t* backButton =
        lv_btn_create(screen);

    lv_obj_set_pos(
        backButton,
        8,
        Theme::CONTENT_TOP);

    lv_obj_set_size(
        backButton,
        116,
        36);

    lv_obj_set_style_bg_color(
        backButton,
        Theme::color(Theme::COLOR_PANEL),
        LV_PART_MAIN);

    lv_obj_set_style_border_color(
        backButton,
        Theme::color(Theme::COLOR_PANEL_BORDER),
        LV_PART_MAIN);

    lv_obj_set_style_border_width(
        backButton,
        1,
        LV_PART_MAIN);

    lv_obj_add_event_cb(
        backButton,
        backButtonEventHandler,
        LV_EVENT_CLICKED,
        this);

    lv_obj_t* backLabel =
        Theme::createLabel(
            backButton,
            LV_SYMBOL_LEFT " DASHBOARD",
            Theme::COLOR_PRIMARY);

    lv_obj_center(backLabel);

    lv_obj_t* pageTitle =
        Theme::createLabel(
            screen,
            "WEATHER DETAIL",
            Theme::COLOR_PRIMARY,
            &lv_font_montserrat_14);

    lv_obj_align(
        pageTitle,
        LV_ALIGN_TOP_LEFT,
        140,
        Theme::CONTENT_TOP + 10);

    lv_obj_t* conditionsPanel = Theme::createPanel(
        screen, 8, 110, 246, 328, "CURRENT CONDITIONS");
    weatherIcon = DashboardIcons::create(
        conditionsPanel, DashboardIcons::Type::Weather, 10, 44, 0xFFFFFF);
    temperatureLabel = Theme::createLabel(
        conditionsPanel, "-- F", Theme::COLOR_TEXT, &lv_font_montserrat_28);
    lv_obj_set_pos(temperatureLabel, 94, 48);
    conditionLabel = Theme::createLabel(
        conditionsPanel, "WAITING FOR WEATHER", Theme::COLOR_TEXT);
    lv_obj_set_width(conditionLabel, 218);
    lv_obj_set_style_text_align(conditionLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_pos(conditionLabel, 0, 116);
    Theme::createDivider(conditionsPanel, 0, 148, 218);
    conditionsDetailLabel = Theme::createLabel(
        conditionsPanel, "HIGH -- F     LOW -- F\n\nHUMIDITY --%\n\nWIND -- mph",
        Theme::COLOR_TEXT_MUTED);
    lv_obj_set_pos(conditionsDetailLabel, 4, 164);
    lv_obj_set_width(conditionsDetailLabel, 210);

    lv_obj_t* atmospherePanel = Theme::createPanel(
        screen, 262, 110, 246, 328, "AIR & ATMOSPHERE");
    airQualityLabel = Theme::createLabel(
        atmospherePanel, "AQI --", Theme::COLOR_PRIMARY, &lv_font_montserrat_28);
    lv_obj_align(airQualityLabel, LV_ALIGN_TOP_MID, 0, 42);
    airQualityCategoryLabel = Theme::createLabel(
        atmospherePanel, "AIR QUALITY UPDATING", Theme::COLOR_TEXT_MUTED);
    lv_obj_set_width(airQualityCategoryLabel, 218);
    lv_obj_set_style_text_align(airQualityCategoryLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_pos(airQualityCategoryLabel, 0, 86);
    Theme::createDivider(atmospherePanel, 0, 120, 218);
    atmosphereDetailLabel = Theme::createLabel(
        atmospherePanel,
        "PM2.5  --\n\nPM10   --\n\nPRESSURE  --.-- inHg\n\nDEW POINT  -- F",
        Theme::COLOR_TEXT_MUTED);
    lv_obj_set_pos(atmosphereDetailLabel, 4, 138);
    lv_obj_set_width(atmosphereDetailLabel, 210);

    forecastPanel.create(
        screen,
        weatherServiceReference,
        516,
        110,
        276,
        328);

    update();

    updateTimer = lv_timer_create(
        updateTimerCallback,
        1000,
        this);

}

void WeatherScreen::show()
{
    if (screen == nullptr)
    {
        return;
    }

    lv_scr_load(screen);
    update();
}

void WeatherScreen::setNavigationCallback(
    NavigationCallback callback)
{
    navigationCallback = callback;
}

void WeatherScreen::update()
{
    headerBar.update();
    if (NetworkUpdateState::isBusy()) return;
    forecastPanel.update();

    if (weatherService == nullptr || temperatureLabel == nullptr) return;
    if (!weatherService->isValid())
    {
        lv_label_set_text(temperatureLabel, "-- F");
        lv_label_set_text(conditionLabel,
            weatherService->isUpdating() ? "UPDATING WEATHER" : "WEATHER UNAVAILABLE");
        return;
    }

    const WeatherData& weather = weatherService->getCurrentWeather();
    const ForecastData& forecast = weatherService->getForecast();
    lv_label_set_text(temperatureLabel, weatherService->getTemperature().c_str());
    lv_label_set_text(conditionLabel, weather.condition.c_str());
    DashboardIcons::setWeatherCondition(weatherIcon, weather.condition);

    String conditions = "HIGH ";
    conditions += forecast.hasHigh ? String(forecast.highF) + " F" : "-- F";
    conditions += "     LOW ";
    conditions += forecast.hasLow ? String(forecast.lowF) + " F" : "-- F";
    conditions += "\n\nHUMIDITY  " + weatherService->getHumidity();
    conditions += "\n\nWIND  " + weatherService->getWind();
    conditions += "\n\nSTATION  " + weatherService->getStationId();
    lv_label_set_text(conditionsDetailLabel, conditions.c_str());

    if (weather.airQualityValid)
    {
        lv_label_set_text(airQualityLabel, (String("AQI ") + weather.usAqi).c_str());
        const String category = aqiCategory(weather.usAqi);
        lv_label_set_text(airQualityCategoryLabel, category.c_str());
        uint32_t categoryColor = Theme::COLOR_SUCCESS;
        if (weather.usAqi > 100) categoryColor = Theme::COLOR_ERROR;
        else if (weather.usAqi > 50) categoryColor = Theme::COLOR_WARNING;
        lv_obj_set_style_text_color(
            airQualityCategoryLabel, Theme::color(categoryColor), LV_PART_MAIN);
    }
    else
    {
        lv_label_set_text(airQualityLabel, "AQI --");
        lv_label_set_text(airQualityCategoryLabel, "AIR QUALITY UNAVAILABLE");
        lv_obj_set_style_text_color(
            airQualityCategoryLabel, Theme::color(Theme::COLOR_TEXT_MUTED), LV_PART_MAIN);
    }

    String atmosphere = "PM2.5  " + valueOrDash(weather.pm25, 1, " ug/m3") +
        "\n\nPM10   " + valueOrDash(weather.pm10, 1, " ug/m3") +
        "\n\nPRESSURE  " + weatherService->getPressure() +
        "\n\nDEW POINT  " + weatherService->getDewPoint();
    lv_label_set_text(atmosphereDetailLabel, atmosphere.c_str());
}

void WeatherScreen::backButtonEventHandler(
    lv_event_t* event)
{
    WeatherScreen* weatherScreen =
        static_cast<WeatherScreen*>(
            lv_event_get_user_data(event));

    if (weatherScreen == nullptr ||
        weatherScreen->navigationCallback == nullptr)
    {
        return;
    }

    weatherScreen->navigationCallback(
        Page::Dashboard);
}

void WeatherScreen::updateTimerCallback(
    lv_timer_t* timer)
{
    WeatherScreen* weatherScreen =
        static_cast<WeatherScreen*>(
            timer->user_data);

    if (weatherScreen == nullptr)
    {
        return;
    }

    weatherScreen->update();
}
