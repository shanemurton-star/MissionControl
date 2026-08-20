#include "WeatherForecastPanel.h"

#include "Theme.h"

void WeatherForecastPanel::create(
    lv_obj_t* parent,
    WeatherService& weatherServiceReference,
    int16_t x,
    int16_t y,
    int16_t width,
    int16_t height)
{
    weatherService = &weatherServiceReference;

    panel = Theme::createPanel(
        parent,
        x,
        y,
        width,
        height,
        "EXTENDED FORECAST  |  ALERTS");

    lv_obj_t* scroller = lv_obj_create(panel);
    lv_obj_set_pos(scroller, 0, 28);
    lv_obj_set_size(scroller, width - 22, height - 42);
    lv_obj_set_style_bg_opa(scroller, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(scroller, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(scroller, 0, LV_PART_MAIN);
    lv_obj_set_scroll_dir(scroller, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(scroller, LV_SCROLLBAR_MODE_AUTO);

    contentLabel = Theme::createLabel(
        scroller,
        "Waiting for forecast data",
        Theme::COLOR_TEXT_MUTED,
        &lv_font_montserrat_14);
    lv_obj_set_width(contentLabel, width - 34);
    lv_label_set_long_mode(contentLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(contentLabel, 0, 0);

    update();
}

void WeatherForecastPanel::update()
{
    if (weatherService == nullptr ||
        contentLabel == nullptr)
    {
        return;
    }

    const ForecastData& forecast =
        weatherService->getForecast();

    if (!forecast.valid)
    {
        lv_label_set_text(
            contentLabel,
            weatherService->isUpdating()
                ? "Updating forecast..."
                : "Forecast unavailable");
        return;
    }

    String content = "HIGH ";
    content += forecast.hasHigh
        ? String(forecast.highF) + " F"
        : "-- F";
    content += "     LOW ";
    content += forecast.hasLow
        ? String(forecast.lowF) + " F"
        : "-- F";

    for (size_t index = 0; index < forecast.periodCount; index++)
    {
        const ForecastPeriod& period =
            forecast.periods[index];

        content += "\n\n";
        content += shorten(period.name, 18);
        content += "  ";
        content += String(period.temperatureF);
        content += " F\n";
        content += shorten(period.shortForecast, 29);
    }

    if (forecast.alertCount == 0)
    {
        content += "\n\nALERTS  NONE ACTIVE";
    }
    else
    {
        content += "\n\nALERT  " + shorten(forecast.primaryAlert, 25);

        if (forecast.alertCount > 1)
        {
            content += " +";
            content += String(forecast.alertCount - 1);
        }
    }

    lv_label_set_text(contentLabel, content.c_str());
}

String WeatherForecastPanel::shorten(
    const String& value,
    size_t maximumLength)
{
    if (value.length() <= maximumLength)
    {
        return value;
    }

    if (maximumLength < 4)
    {
        return value.substring(0, maximumLength);
    }

    return value.substring(0, maximumLength - 3) +
           "...";
}
