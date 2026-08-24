#include "WeatherPanel.h"

#include <WiFi.h>
#include "DashboardIcons.h"
#include "Theme.h"

namespace
{
    lv_color_t colorFromHex(
        uint32_t value)
    {
        return lv_color_hex(value);
    }

    lv_obj_t* createLabel(
        lv_obj_t* parent,
        const char* text,
        uint32_t color)
    {
        lv_obj_t* label =
            lv_label_create(parent);

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

    void configurePanel(
        lv_obj_t* panel)
    {
        Theme::configurePanel(panel);
    }
}

void WeatherPanel::create(
    lv_obj_t* parent,
    WeatherService& weatherServiceReference,
    int16_t x,
    int16_t y,
    int16_t width,
    int16_t height)
{
    weatherService =
        &weatherServiceReference;

    panel =
        lv_obj_create(parent);

    lv_obj_set_pos(
        panel,
        x,
        y);

    lv_obj_set_size(
        panel,
        width,
        height);

    configurePanel(panel);

    lv_obj_add_flag(
        panel,
        LV_OBJ_FLAG_CLICKABLE);

    lv_obj_add_event_cb(
        panel,
        panelEventHandler,
        LV_EVENT_CLICKED,
        this);

    lv_obj_t* titleLabel =
        createLabel(
            panel,
            "WEATHER",
            0xFFFFFF);

    lv_obj_align(
        titleLabel,
        LV_ALIGN_TOP_MID,
        0,
        0);

    weatherIcon = DashboardIcons::create(
        panel, DashboardIcons::Type::Weather, 12, 42, 0xFFFFFF);

    conditionLabel =
        createLabel(
            panel,
            "WAITING FOR WEATHER",
            0xFFFFFF);

    lv_obj_align(
        conditionLabel,
        LV_ALIGN_TOP_MID,
        35,
        98);

    temperatureLabel =
        createLabel(
            panel,
            "-- F",
            0xFFFFFF);
    lv_obj_set_style_text_font(temperatureLabel, &lv_font_montserrat_28, LV_PART_MAIN);

    lv_obj_align(
        temperatureLabel,
        LV_ALIGN_TOP_MID,
        38,
        47);

    detailsLabel =
        createLabel(
            panel,
            "Humidity --%",
            0x8EA9C1);

    lv_obj_align(
        detailsLabel,
        LV_ALIGN_BOTTOM_MID,
        35,
        -4);

    update();
}

void WeatherPanel::update()
{
    if (weatherService == nullptr ||
        conditionLabel == nullptr ||
        temperatureLabel == nullptr ||
        detailsLabel == nullptr)
    {
        return;
    }

    const bool wifiConnected =
        WiFi.status() == WL_CONNECTED;

    if (!wifiConnected)
    {
        lv_label_set_text(
            conditionLabel,
            "WAITING FOR WIFI");

        lv_label_set_text(
            temperatureLabel,
            "-- F");

        lv_label_set_text(
            detailsLabel,
            "Humidity --%");

        lv_obj_set_style_text_color(
            conditionLabel,
            colorFromHex(0xF4C95D),
            LV_PART_MAIN);

        return;
    }

    if (weatherService->isValid())
    {
        const WeatherData& weather =
            weatherService->getCurrentWeather();

        const String temperature =
            weatherService->getTemperature();

        const String humidity =
            weatherService->getHumidity();

        lv_label_set_text(
            conditionLabel,
            weather.condition.c_str());

        DashboardIcons::setWeatherCondition(weatherIcon, weather.condition);

        lv_label_set_text(
            temperatureLabel,
            temperature.c_str());

        String details;
        details.reserve(32);
        details += "Humidity ";
        details += humidity;

        lv_label_set_text(
            detailsLabel,
            details.c_str());

        lv_obj_set_style_text_color(
            conditionLabel,
            colorFromHex(0xFFFFFF),
            LV_PART_MAIN);

        return;
    }

    if (weatherService->isUpdating())
    {
        lv_label_set_text(
            conditionLabel,
            "UPDATING WEATHER");

        lv_label_set_text(
            temperatureLabel,
            "-- F");

        lv_label_set_text(
            detailsLabel,
            "Contacting National\n"
            "Weather Service...\n\n"
            "Please wait");

        lv_obj_set_style_text_color(
            conditionLabel,
            colorFromHex(0xF4C95D),
            LV_PART_MAIN);

        return;
    }

    lv_label_set_text(
        conditionLabel,
        "WEATHER UNAVAILABLE");

    lv_label_set_text(
        temperatureLabel,
        "-- F");

    const String errorMessage =
        weatherService->getLastError();

    if (errorMessage.isEmpty())
    {
        lv_label_set_text(
            detailsLabel,
            "Waiting for weather\n"
            "service to update...");
    }
    else
    {
        lv_label_set_text(
            detailsLabel,
            errorMessage.c_str());
    }

    lv_obj_set_style_text_color(
        conditionLabel,
        colorFromHex(0xF4C95D),
        LV_PART_MAIN);
}

void WeatherPanel::setClickCallback(
    ClickCallback callback)
{
    clickCallback = callback;
}

void WeatherPanel::panelEventHandler(
    lv_event_t* event)
{
    WeatherPanel* weatherPanel =
        static_cast<WeatherPanel*>(
            lv_event_get_user_data(event));

    if (weatherPanel == nullptr ||
        weatherPanel->clickCallback == nullptr)
    {
        return;
    }

    weatherPanel->clickCallback();
}
