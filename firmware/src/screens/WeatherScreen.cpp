#include "WeatherScreen.h"
#include "../services/NetworkUpdateState.h"

#include <math.h>
#include <time.h>
#include "../ui/DashboardIcons.h"
#include "../ui/Theme.h"

namespace
{
    struct DailyForecastSummary
    {
        bool hasHigh = false;
        bool hasLow = false;
        int16_t highF = 0;
        int16_t lowF = 0;
        String condition;
    };

    double normalizeDegrees(double value)
    {
        while (value < 0.0) value += 360.0;
        while (value >= 360.0) value -= 360.0;
        return value;
    }

    bool solarUtcHour(
        int dayOfYear,
        double latitude,
        double longitude,
        bool sunrise,
        double& utcHour)
    {
        constexpr double ZENITH = 90.833;
        const double longitudeHour = longitude / 15.0;
        const double approximateTime = dayOfYear +
            ((sunrise ? 6.0 : 18.0) - longitudeHour) / 24.0;
        const double meanAnomaly = 0.9856 * approximateTime - 3.289;
        double trueLongitude = meanAnomaly +
            1.916 * sin(meanAnomaly * DEG_TO_RAD) +
            0.020 * sin(2.0 * meanAnomaly * DEG_TO_RAD) + 282.634;
        trueLongitude = normalizeDegrees(trueLongitude);

        double rightAscension = atan(0.91764 *
            tan(trueLongitude * DEG_TO_RAD)) / DEG_TO_RAD;
        rightAscension = normalizeDegrees(rightAscension);
        rightAscension +=
            floor(trueLongitude / 90.0) * 90.0 -
            floor(rightAscension / 90.0) * 90.0;
        rightAscension /= 15.0;

        const double sinDeclination =
            0.39782 * sin(trueLongitude * DEG_TO_RAD);
        const double cosDeclination = cos(asin(sinDeclination));
        const double latitudeRadians = latitude * DEG_TO_RAD;
        const double cosHour =
            (cos(ZENITH * DEG_TO_RAD) -
             sinDeclination * sin(latitudeRadians)) /
            (cosDeclination * cos(latitudeRadians));
        if (cosHour < -1.0 || cosHour > 1.0) return false;

        double localHourAngle = acos(cosHour) / DEG_TO_RAD;
        if (sunrise) localHourAngle = 360.0 - localHourAngle;
        localHourAngle /= 15.0;

        const double localMeanTime = localHourAngle + rightAscension -
            0.06571 * approximateTime - 6.622;
        utcHour = localMeanTime - longitudeHour;
        return true;
    }

    String formatSolarTime(double utcHour, time_t now)
    {
        struct tm localInfo;
        struct tm utcInfo;
        localtime_r(&now, &localInfo);
        gmtime_r(&now, &utcInfo);
        const time_t utcInterpretedAsLocal = mktime(&utcInfo);
        const double utcOffsetHours =
            difftime(now, utcInterpretedAsLocal) / 3600.0;
        double localHour = fmod(utcHour + utcOffsetHours, 24.0);
        if (localHour < 0.0) localHour += 24.0;

        int totalMinutes = static_cast<int>(round(localHour * 60.0)) % 1440;
        int hour = totalMinutes / 60;
        const int minute = totalMinutes % 60;
        const bool afternoon = hour >= 12;
        int displayHour = hour % 12;
        if (displayHour == 0) displayHour = 12;
        char buffer[12];
        snprintf(
            buffer, sizeof(buffer), "%d:%02d %s",
            displayHour, minute, afternoon ? "PM" : "AM");
        return String(buffer);
    }

    String moonPhaseName(time_t now)
    {
        // 2000-01-06 18:14 UTC was a well-established astronomical new moon.
        constexpr double NEW_MOON_EPOCH = 947182440.0;
        constexpr double SYNODIC_MONTH_SECONDS = 29.53058867 * 86400.0;
        double cycle = fmod(
            difftime(now, static_cast<time_t>(NEW_MOON_EPOCH)),
            SYNODIC_MONTH_SECONDS);
        if (cycle < 0.0) cycle += SYNODIC_MONTH_SECONDS;
        const uint8_t phase = static_cast<uint8_t>(
            floor(cycle / SYNODIC_MONTH_SECONDS * 8.0 + 0.5)) % 8;
        const char* names[8] = {
            "NEW MOON", "WAXING CRESCENT", "FIRST QUARTER",
            "WAXING GIBBOUS", "FULL MOON", "WANING GIBBOUS",
            "LAST QUARTER", "WANING CRESCENT"};
        return String(names[phase]);
    }

    String formatHourlyTime(const String& startTime)
    {
        if (startTime.length() < 13) return "--";
        int hour = startTime.substring(11, 13).toInt();
        const bool afternoon = hour >= 12;
        hour %= 12;
        if (hour == 0) hour = 12;
        return String(hour) + (afternoon ? " PM" : " AM");
    }

    lv_obj_t* createInstrumentArc(
        lv_obj_t* parent,
        int16_t x,
        int16_t y,
        int16_t size,
        uint32_t color,
        int16_t minimum,
        int16_t maximum)
    {
        lv_obj_t* arc = lv_arc_create(parent);
        lv_obj_set_pos(arc, x, y);
        lv_obj_set_size(arc, size, size);
        lv_arc_set_bg_angles(arc, 135, 45);
        lv_arc_set_range(arc, minimum, maximum);
        lv_arc_set_value(arc, minimum);
        lv_obj_remove_style(arc, nullptr, LV_PART_KNOB);
        lv_obj_set_style_arc_width(arc, 8, LV_PART_MAIN);
        lv_obj_set_style_arc_color(
            arc, Theme::color(Theme::COLOR_PANEL_BORDER), LV_PART_MAIN);
        lv_obj_set_style_arc_width(arc, 8, LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(arc, Theme::color(color), LV_PART_INDICATOR);
        lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
        return arc;
    }

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

    void createVerticalDivider(
        lv_obj_t* parent, int16_t x, int16_t y, int16_t height)
    {
        lv_obj_t* divider = lv_obj_create(parent);
        lv_obj_set_pos(divider, x, y);
        lv_obj_set_size(divider, 1, height);
        lv_obj_set_style_bg_color(
            divider, Theme::color(Theme::COLOR_PANEL_BORDER), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(divider, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(divider, 0, LV_PART_MAIN);
        lv_obj_clear_flag(divider, LV_OBJ_FLAG_SCROLLABLE);
    }

    void summarizeForecast(
        const ForecastData& forecast, DailyForecastSummary summaries[2])
    {
        uint8_t day = 0;
        bool dayHasPeriod = false;

        for (size_t index = 0;
             index < forecast.periodCount && day < 2;
             ++index)
        {
            const ForecastPeriod& period = forecast.periods[index];

            // A new daytime period starts the next daily pair. When the feed
            // begins at night, that first night remains in Today's card.
            if (period.daytime && dayHasPeriod)
            {
                ++day;
                dayHasPeriod = false;
                if (day >= 2) break;
            }

            DailyForecastSummary& summary = summaries[day];
            dayHasPeriod = true;
            if (period.daytime)
            {
                summary.highF = period.temperatureF;
                summary.hasHigh = true;
                summary.condition = period.shortForecast;
            }
            else
            {
                summary.lowF = period.temperatureF;
                summary.hasLow = true;
                if (summary.condition.isEmpty())
                    summary.condition = period.shortForecast;
            }
        }

        // Once the NWS feed rolls over to "Tonight", it no longer includes
        // today's daytime period. Preserve the known high retained by the
        // service so navigating or refreshing after sunset does not blank it.
        if (!summaries[0].hasHigh && forecast.hasHigh)
        {
            summaries[0].highF = forecast.highF;
            summaries[0].hasHigh = true;
        }
        if (!summaries[0].hasLow && forecast.hasLow)
        {
            summaries[0].lowF = forecast.lowF;
            summaries[0].hasLow = true;
        }
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
    headerBar.useLocationIdentity();

    lv_obj_t* conditionsPanel = Theme::createPanel(
        screen, 8, 72, 250, 238, "TEMPERATURE  |  -20 TO 120 F");
    temperatureArc = createInstrumentArc(
        conditionsPanel, 40, 35, 148, Theme::COLOR_WARNING, -20, 120);
    weatherIcon = DashboardIcons::create(
        conditionsPanel, DashboardIcons::Type::Weather, 48, 73, 0xFFFFFF);
    lv_img_set_zoom(weatherIcon, 208);
    temperatureLabel = Theme::createLabel(
        conditionsPanel, "-- F", Theme::COLOR_TEXT, &lv_font_montserrat_28);
    lv_obj_set_width(temperatureLabel, 92);
    lv_obj_set_style_text_align(temperatureLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_pos(temperatureLabel, 96, 76);
    conditionLabel = Theme::createLabel(
        conditionsPanel, "WAITING FOR WEATHER", Theme::COLOR_TEXT);
    lv_obj_set_width(conditionLabel, 220);
    lv_obj_set_style_text_align(conditionLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_pos(conditionLabel, 0, 166);
    highLowLabel = Theme::createLabel(
        conditionsPanel, "HIGH -- F     LOW -- F", Theme::COLOR_TEXT_MUTED);
    lv_obj_set_width(highLowLabel, 220);
    lv_obj_set_style_text_align(highLowLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_pos(highLowLabel, 0, 193);

    lv_obj_t* windPanel = Theme::createPanel(
        screen, 266, 72, 250, 238, "WIND  |  0 TO 50 MPH");
    windArc = createInstrumentArc(
        windPanel, 40, 35, 148, Theme::COLOR_PRIMARY, 0, 50);
    windDirectionLabel = Theme::createLabel(
        windPanel, "--", Theme::COLOR_PRIMARY, &lv_font_montserrat_20);
    lv_obj_set_width(windDirectionLabel, 220);
    lv_obj_set_style_text_align(windDirectionLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_pos(windDirectionLabel, 0, 137);
    windSpeedLabel = Theme::createLabel(
        windPanel, "--", Theme::COLOR_TEXT, &lv_font_montserrat_28);
    lv_obj_set_width(windSpeedLabel, 220);
    lv_obj_set_style_text_align(windSpeedLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_pos(windSpeedLabel, 0, 67);
    lv_obj_t* mphLabel = Theme::createLabel(windPanel, "mph", Theme::COLOR_TEXT_MUTED);
    lv_obj_align(mphLabel, LV_ALIGN_TOP_MID, 0, 105);
    windGustLabel = Theme::createLabel(
        windPanel, "GUST -- mph", Theme::COLOR_TEXT_MUTED);
    lv_obj_set_width(windGustLabel, 220);
    lv_obj_set_style_text_align(windGustLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_pos(windGustLabel, 0, 193);

    lv_obj_t* airPanel = Theme::createPanel(
        screen, 524, 72, 268, 238, "HUMIDITY %  |  AQI 0-300");
    humidityArc = createInstrumentArc(
        airPanel, 4, 52, 112, 0x8B66E8, 0, 100);
    airQualityArc = createInstrumentArc(
        airPanel, 132, 52, 112, Theme::COLOR_SUCCESS, 0, 300);
    humidityLabel = Theme::createLabel(
        airPanel, "--%", Theme::COLOR_TEXT, &lv_font_montserrat_20);
    lv_obj_set_width(humidityLabel, 112);
    lv_obj_set_style_text_align(humidityLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_pos(humidityLabel, 4, 92);
    lv_obj_t* humidityCaption = Theme::createLabel(
        airPanel, "HUMIDITY", Theme::COLOR_TEXT_MUTED);
    lv_obj_set_width(humidityCaption, 112);
    lv_obj_set_style_text_align(humidityCaption, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_pos(humidityCaption, 4, 181);
    airQualityLabel = Theme::createLabel(
        airPanel, "--", Theme::COLOR_TEXT, &lv_font_montserrat_20);
    lv_obj_set_width(airQualityLabel, 112);
    lv_obj_set_style_text_align(airQualityLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_pos(airQualityLabel, 132, 92);
    airQualityCategoryLabel = Theme::createLabel(
        airPanel, "AQI UPDATING", Theme::COLOR_TEXT_MUTED);
    lv_obj_set_width(airQualityCategoryLabel, 122);
    lv_obj_set_style_text_align(airQualityCategoryLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_pos(airQualityCategoryLabel, 127, 181);

    lv_obj_t* summaryPanel = Theme::createPanel(
        screen, 8, 318, 784, 120, "");
    atmosphereDetailLabel = Theme::createLabel(
        summaryPanel,
        "DEW -- F   PRESSURE --.-- inHg   UV INDEX --",
        Theme::COLOR_TEXT_MUTED);
    lv_obj_set_pos(atmosphereDetailLabel, 0, 79);
    lv_obj_set_width(atmosphereDetailLabel, 430);
    constexpr int16_t dayWidth = 378;
    constexpr int16_t valuesWidth = 120;
    for (uint8_t index = 0; index < 2; ++index)
    {
        const int16_t dayX = index * dayWidth;
        const int16_t conditionX = dayX + valuesWidth + 4;

        dailyForecastLabels[index] = Theme::createLabel(
            summaryPanel,
            index == 0 ? "TODAY" : "TOMORROW",
            Theme::COLOR_TEXT);
        lv_obj_set_pos(dailyForecastLabels[index], dayX, 1);
        lv_obj_set_width(dailyForecastLabels[index], dayWidth - 10);
        lv_obj_set_style_text_align(
            dailyForecastLabels[index], LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

        dailyHighLabels[index] = Theme::createLabel(
            summaryPanel, "HIGH\n-- F", Theme::COLOR_TEXT_MUTED);
        lv_obj_set_pos(dailyHighLabels[index], dayX, 22);
        lv_obj_set_width(dailyHighLabels[index], valuesWidth / 2 - 4);
        lv_obj_set_height(dailyHighLabels[index], 40);
        lv_obj_set_style_text_align(
            dailyHighLabels[index], LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

        dailyLowLabels[index] = Theme::createLabel(
            summaryPanel, "LOW\n-- F", Theme::COLOR_TEXT_MUTED);
        lv_obj_set_pos(
            dailyLowLabels[index], dayX + valuesWidth / 2 + 4, 22);
        lv_obj_set_width(dailyLowLabels[index], valuesWidth / 2 - 4);
        lv_obj_set_height(dailyLowLabels[index], 40);
        lv_obj_set_style_text_align(
            dailyLowLabels[index], LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

        createVerticalDivider(
            summaryPanel, dayX + valuesWidth / 2, 24, 36);

        dailyForecastIcons[index] = DashboardIcons::create(
            summaryPanel,
            DashboardIcons::Type::Weather,
            conditionX,
            14,
            0xFFFFFF);
        lv_img_set_zoom(dailyForecastIcons[index], 224);
        dailyConditionLabels[index] = Theme::createLabel(
            summaryPanel, "UPDATING", Theme::COLOR_TEXT_MUTED);
        lv_obj_set_pos(dailyConditionLabels[index], conditionX + 69, 20);
        lv_obj_set_width(dailyConditionLabels[index], dayWidth - valuesWidth - 83);
        lv_obj_set_height(dailyConditionLabels[index], 56);
        lv_obj_set_style_text_align(
            dailyConditionLabels[index], LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_label_set_long_mode(dailyConditionLabels[index], LV_LABEL_LONG_WRAP);
    }
    createVerticalDivider(summaryPanel, dayWidth - 5, 20, 49);
    alertLabel = Theme::createLabel(summaryPanel, "", Theme::COLOR_WARNING);
    lv_obj_set_pos(alertLabel, 430, 79);
    lv_obj_set_width(alertLabel, 326);
    lv_obj_set_style_text_align(alertLabel, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);

    // A transparent touch target preserves the clean forecast layout while
    // making the entire Today half easy to open on a physical touchscreen.
    hourlyForecastTarget = lv_btn_create(summaryPanel);
    lv_obj_set_pos(hourlyForecastTarget, 0, 0);
    lv_obj_set_size(hourlyForecastTarget, dayWidth - 5, 75);
    lv_obj_set_style_bg_opa(
        hourlyForecastTarget, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_bg_color(
        hourlyForecastTarget, Theme::color(Theme::COLOR_PRIMARY),
        LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(
        hourlyForecastTarget, LV_OPA_20,
        LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_width(hourlyForecastTarget, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(hourlyForecastTarget, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(hourlyForecastTarget, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(
        hourlyForecastTarget,
        hourlyForecastEventHandler,
        LV_EVENT_CLICKED,
        this);

    update();

    updateTimer = lv_timer_create(
        updateTimerCallback,
        1000,
        this);

}

void WeatherScreen::createHourlyScreen()
{
    if (hourlyScreen != nullptr || clockService == nullptr) return;

    hourlyScreen = lv_obj_create(nullptr);
    Theme::configureScreen(hourlyScreen);
    hourlyHeaderBar.create(
        hourlyScreen, *clockService, Theme::SCREEN_WIDTH, Theme::HEADER_HEIGHT);
    hourlyHeaderBar.useLocationIdentity();
    hourlyHeaderBar.setSettingsCallback([this]() {
        if (navigationCallback != nullptr) navigationCallback(Page::Settings);
    });
    hourlyHeaderBar.setNavigationCallback([this](Page page) {
        if (navigationCallback != nullptr) navigationCallback(page);
    });

    lv_obj_t* backButton = lv_btn_create(hourlyScreen);
    lv_obj_set_pos(backButton, 8, Theme::CONTENT_TOP);
    lv_obj_set_size(backButton, 116, 30);
    lv_obj_set_style_bg_color(
        backButton, Theme::color(Theme::COLOR_PANEL), LV_PART_MAIN);
    lv_obj_set_style_border_color(
        backButton, Theme::color(Theme::COLOR_PANEL_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(backButton, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(backButton, 8, LV_PART_MAIN);
    lv_obj_add_event_cb(
        backButton, hourlyBackEventHandler, LV_EVENT_CLICKED, this);
    lv_obj_center(Theme::createLabel(
        backButton, LV_SYMBOL_LEFT " WEATHER", Theme::COLOR_PRIMARY));

    lv_obj_t* title = Theme::createLabel(
        hourlyScreen, "NEXT 12 HOURS", Theme::COLOR_PRIMARY);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 144, Theme::CONTENT_TOP + 8);

    hourlyScroller = lv_obj_create(hourlyScreen);
    lv_obj_set_pos(hourlyScroller, 8, 110);
    lv_obj_set_size(hourlyScroller, 784, 328);
    lv_obj_set_style_bg_opa(hourlyScroller, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(hourlyScroller, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(hourlyScroller, 0, LV_PART_MAIN);
    lv_obj_set_scroll_dir(hourlyScroller, LV_DIR_HOR);
    lv_obj_set_scrollbar_mode(hourlyScroller, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_snap_x(hourlyScroller, LV_SCROLL_SNAP_START);

    constexpr int16_t CARD_WIDTH = 124;
    constexpr int16_t CARD_GAP = 7;
    for (uint8_t index = 0;
         index < ForecastData::MAX_HOURLY_PERIODS;
         ++index)
    {
        lv_obj_t* card = Theme::createPanel(
            hourlyScroller,
            index * (CARD_WIDTH + CARD_GAP),
            0,
            CARD_WIDTH,
            302,
            "");
        hourlyCards[index] = card;

        hourlyTimeLabels[index] = Theme::createLabel(
            card, "--", Theme::COLOR_PRIMARY, &lv_font_montserrat_20);
        lv_obj_set_pos(hourlyTimeLabels[index], 0, 2);
        lv_obj_set_width(hourlyTimeLabels[index], 102);
        lv_obj_set_style_text_align(
            hourlyTimeLabels[index], LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

        hourlyIcons[index] = DashboardIcons::create(
            card, DashboardIcons::Type::Weather, 29, 43, 0xFFFFFF);
        lv_img_set_zoom(hourlyIcons[index], 224);

        hourlyTemperatureLabels[index] = Theme::createLabel(
            card, "-- F", Theme::COLOR_TEXT, &lv_font_montserrat_28);
        lv_obj_set_pos(hourlyTemperatureLabels[index], 0, 105);
        lv_obj_set_width(hourlyTemperatureLabels[index], 102);
        lv_obj_set_style_text_align(
            hourlyTemperatureLabels[index], LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

        hourlyConditionLabels[index] = Theme::createLabel(
            card, "UPDATING", Theme::COLOR_TEXT_MUTED);
        lv_obj_set_pos(hourlyConditionLabels[index], 0, 145);
        lv_obj_set_size(hourlyConditionLabels[index], 102, 62);
        lv_obj_set_style_text_align(
            hourlyConditionLabels[index], LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_label_set_long_mode(
            hourlyConditionLabels[index], LV_LABEL_LONG_WRAP);

        hourlyPrecipitationLabels[index] = Theme::createLabel(
            card, "PRECIP --%", Theme::COLOR_PRIMARY);
        lv_obj_set_pos(hourlyPrecipitationLabels[index], 0, 219);
        lv_obj_set_width(hourlyPrecipitationLabels[index], 102);
        lv_obj_set_style_text_align(
            hourlyPrecipitationLabels[index], LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

        hourlyWindLabels[index] = Theme::createLabel(
            card, "WIND --", Theme::COLOR_TEXT_DIM);
        lv_obj_set_pos(hourlyWindLabels[index], 0, 249);
        lv_obj_set_size(hourlyWindLabels[index], 102, 40);
        lv_obj_set_style_text_align(
            hourlyWindLabels[index], LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_label_set_long_mode(hourlyWindLabels[index], LV_LABEL_LONG_WRAP);
    }

    hourlyStatusLabel = Theme::createLabel(
        hourlyScreen, "HOURLY FORECAST UPDATING", Theme::COLOR_WARNING);
    lv_obj_align(hourlyStatusLabel, LV_ALIGN_CENTER, 0, 20);
    lv_obj_add_flag(hourlyStatusLabel, LV_OBJ_FLAG_HIDDEN);
}

void WeatherScreen::showHourlyScreen()
{
    createHourlyScreen();
    if (hourlyScreen == nullptr) return;
    updateHourlyForecast();
    lv_scr_load(hourlyScreen);
}

void WeatherScreen::updateHourlyForecast()
{
    if (hourlyScreen == nullptr || weatherService == nullptr) return;
    const uint32_t revision = weatherService->getDataRevision();
    if (revision == renderedHourlyRevision) return;
    renderedHourlyRevision = revision;

    const ForecastData& forecast = weatherService->getForecast();
    if (!forecast.hourlyValid || forecast.hourlyPeriodCount == 0)
    {
        lv_obj_add_flag(hourlyScroller, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(hourlyStatusLabel, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(
            hourlyStatusLabel,
            weatherService->isUpdating()
                ? "HOURLY FORECAST UPDATING"
                : "HOURLY FORECAST UNAVAILABLE");
        return;
    }

    lv_obj_add_flag(hourlyStatusLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(hourlyScroller, LV_OBJ_FLAG_HIDDEN);
    for (uint8_t index = 0;
         index < ForecastData::MAX_HOURLY_PERIODS;
         ++index)
    {
        if (index >= forecast.hourlyPeriodCount)
        {
            lv_obj_add_flag(hourlyCards[index], LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        const HourlyForecastPeriod& period = forecast.hourlyPeriods[index];
        lv_obj_clear_flag(hourlyCards[index], LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(
            hourlyTimeLabels[index], formatHourlyTime(period.startTime).c_str());
        lv_label_set_text(
            hourlyTemperatureLabels[index],
            (String(period.temperatureF) + " F").c_str());
        lv_label_set_text(
            hourlyConditionLabels[index], period.shortForecast.c_str());
        DashboardIcons::setWeatherCondition(
            hourlyIcons[index], period.shortForecast);
        lv_label_set_text(
            hourlyPrecipitationLabels[index],
            (String("PRECIP ") + period.precipitationPercent + "%").c_str());
        lv_label_set_text(
            hourlyWindLabels[index],
            (period.windDirection + "  " + period.windSpeed).c_str());
    }
}

void WeatherScreen::updateAstronomy()
{
    if (clockService == nullptr || weatherService == nullptr) return;
    const time_t now = time(nullptr);
    if (now < 1704067200) return;

    struct tm localInfo;
    localtime_r(&now, &localInfo);
    // Include DST state so a display left running through the seasonal clock
    // change recalculates the local rise/set times immediately.
    const String dateKey = String(localInfo.tm_year) + ":" +
        localInfo.tm_yday + ":" + localInfo.tm_isdst;
    if (dateKey == renderedAstronomyDate) return;

    double sunriseUtc = 0.0;
    double sunsetUtc = 0.0;
    const bool hasSunrise = solarUtcHour(
        localInfo.tm_yday + 1,
        weatherService->getLatitude(),
        weatherService->getLongitude(),
        true,
        sunriseUtc);
    const bool hasSunset = solarUtcHour(
        localInfo.tm_yday + 1,
        weatherService->getLatitude(),
        weatherService->getLongitude(),
        false,
        sunsetUtc);

    String astronomy = String("SUNRISE  ") +
        (hasSunrise ? formatSolarTime(sunriseUtc, now) : "--") +
        "    SUNSET  " +
        (hasSunset ? formatSolarTime(sunsetUtc, now) : "--") +
        "    MOON  " + moonPhaseName(now);
    headerBar.setSecondaryIdentity(astronomy);
    renderedAstronomyDate = dateKey;
}

void WeatherScreen::hourlyForecastEventHandler(lv_event_t* event)
{
    WeatherScreen* self = static_cast<WeatherScreen*>(
        lv_event_get_user_data(event));
    if (self != nullptr) self->showHourlyScreen();
}

void WeatherScreen::hourlyBackEventHandler(lv_event_t* event)
{
    WeatherScreen* self = static_cast<WeatherScreen*>(
        lv_event_get_user_data(event));
    if (self == nullptr || self->screen == nullptr) return;
    lv_scr_load(self->screen);
    self->update();
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

void WeatherScreen::release()
{
    if (updateTimer != nullptr) { lv_timer_del(updateTimer); updateTimer = nullptr; }
    if (hourlyScreen != nullptr) { lv_obj_del(hourlyScreen); hourlyScreen = nullptr; }
    if (screen != nullptr) { lv_obj_del(screen); screen = nullptr; }
    temperatureArc = windArc = humidityArc = airQualityArc = nullptr;
    weatherIcon = temperatureLabel = conditionLabel = highLowLabel = nullptr;
    windDirectionLabel = windSpeedLabel = windGustLabel = humidityLabel = nullptr;
    airQualityLabel = airQualityCategoryLabel = atmosphereDetailLabel = nullptr;
    for (uint8_t i = 0; i < 2; ++i)
    {
        dailyForecastLabels[i] = nullptr;
        dailyHighLabels[i] = nullptr;
        dailyLowLabels[i] = nullptr;
        dailyForecastIcons[i] = nullptr;
        dailyConditionLabels[i] = nullptr;
    }
    alertLabel = hourlyForecastTarget = hourlyScroller = hourlyStatusLabel = nullptr;
    for (uint8_t i = 0; i < ForecastData::MAX_HOURLY_PERIODS; ++i)
    {
        hourlyCards[i] = hourlyTimeLabels[i] = hourlyIcons[i] = nullptr;
        hourlyTemperatureLabels[i] = hourlyConditionLabels[i] = nullptr;
        hourlyPrecipitationLabels[i] = hourlyWindLabels[i] = nullptr;
    }
    renderedWeatherRevision = UINT32_MAX;
    renderedHourlyRevision = UINT32_MAX;
    renderedWeatherValid = false;
    renderedWeatherUpdating = false;
    renderedAstronomyDate = "";
}

void WeatherScreen::setNavigationCallback(
    NavigationCallback callback)
{
    navigationCallback = callback;
}

void WeatherScreen::update()
{
    // Keep the clock live every second, but redraw the weather instruments only
    // when a network refresh has actually produced new data. Rewriting the same
    // labels, arcs, styles, and PNG sources every second caused broad LVGL
    // invalidation and the visible shimmer on this screen.
    headerBar.update();
    if (hourlyScreen != nullptr && lv_scr_act() == hourlyScreen)
        hourlyHeaderBar.update();
    updateAstronomy();
    if (NetworkUpdateState::isBusy()) return;
    if (weatherService == nullptr || temperatureLabel == nullptr) return;

    const uint32_t weatherRevision = weatherService->getDataRevision();
    const bool weatherValid = weatherService->isValid();
    const bool weatherUpdating = weatherService->isUpdating();
    if (weatherRevision == renderedWeatherRevision &&
        weatherValid == renderedWeatherValid &&
        weatherUpdating == renderedWeatherUpdating)
    {
        return;
    }

    renderedWeatherRevision = weatherRevision;
    renderedWeatherValid = weatherValid;
    renderedWeatherUpdating = weatherUpdating;

    if (!weatherValid)
    {
        lv_label_set_text(temperatureLabel, "-- F");
        lv_label_set_text(conditionLabel,
            weatherUpdating ? "UPDATING WEATHER" : "WEATHER UNAVAILABLE");
        updateHourlyForecast();
        return;
    }

    const WeatherData& weather = weatherService->getCurrentWeather();
    const ForecastData& forecast = weatherService->getForecast();
    lv_label_set_text(temperatureLabel, weatherService->getTemperature().c_str());
    lv_label_set_text(conditionLabel, weather.condition.c_str());
    DashboardIcons::setWeatherCondition(weatherIcon, weather.condition);

    String highLow = "HIGH ";
    highLow += forecast.hasHigh ? String(forecast.highF) + " F" : "-- F";
    highLow += "     LOW ";
    highLow += forecast.hasLow ? String(forecast.lowF) + " F" : "-- F";
    lv_label_set_text(highLowLabel, highLow.c_str());

    if (!isnan(weather.temperatureF))
        lv_arc_set_value(temperatureArc, static_cast<int16_t>(weather.temperatureF));

    lv_label_set_text(windDirectionLabel, weather.windDirection.c_str());
    lv_label_set_text(windSpeedLabel,
        isnan(weather.windSpeedMph) ? "--" : String(weather.windSpeedMph, 0).c_str());
    if (!isnan(weather.windSpeedMph))
        lv_arc_set_value(windArc, static_cast<int16_t>(weather.windSpeedMph));
    const String gust = isnan(weather.windGustMph)
        ? "GUST NOT REPORTED"
        : "GUST " + valueOrDash(weather.windGustMph, 0, " mph");
    lv_label_set_text(windGustLabel, gust.c_str());
    lv_label_set_text(humidityLabel,
        isnan(weather.humidityPercent) ? "--%" : (String(weather.humidityPercent, 0) + "%").c_str());
    if (!isnan(weather.humidityPercent))
        lv_arc_set_value(humidityArc, static_cast<int16_t>(weather.humidityPercent));

    if (weather.airQualityValid)
    {
        lv_label_set_text(airQualityLabel, String(weather.usAqi).c_str());
        const String category = aqiCategory(weather.usAqi);
        lv_label_set_text(airQualityCategoryLabel, category.c_str());
        uint32_t categoryColor = Theme::COLOR_SUCCESS;
        if (weather.usAqi > 100) categoryColor = Theme::COLOR_ERROR;
        else if (weather.usAqi > 50) categoryColor = Theme::COLOR_WARNING;
        lv_obj_set_style_text_color(
            airQualityCategoryLabel, Theme::color(categoryColor), LV_PART_MAIN);
        lv_arc_set_value(airQualityArc, weather.usAqi > 300 ? 300 : weather.usAqi);
        lv_obj_set_style_arc_color(
            airQualityArc, Theme::color(categoryColor), LV_PART_INDICATOR);
    }
    else
    {
        lv_label_set_text(airQualityLabel, "--");
        lv_label_set_text(airQualityCategoryLabel, "AQI UNAVAILABLE");
        lv_obj_set_style_text_color(
            airQualityCategoryLabel, Theme::color(Theme::COLOR_TEXT_MUTED), LV_PART_MAIN);
    }

    String atmosphere = "DEW " + weatherService->getDewPoint() +
        "   PRESSURE " + weatherService->getPressure() +
        "   UV INDEX " + valueOrDash(weather.uvIndex, 1, "");
    lv_label_set_text(atmosphereDetailLabel, atmosphere.c_str());

    DailyForecastSummary dailyForecasts[2];
    summarizeForecast(forecast, dailyForecasts);
    for (size_t index = 0; index < 2; ++index)
    {
        const DailyForecastSummary& summary = dailyForecasts[index];
        const String highText = String("HIGH\n") +
            (summary.hasHigh ? String(summary.highF) + " F" : "-- F");
        const String lowText = String("LOW\n") +
            (summary.hasLow ? String(summary.lowF) + " F" : "-- F");
        lv_label_set_text(dailyHighLabels[index], highText.c_str());
        lv_label_set_text(dailyLowLabels[index], lowText.c_str());

        const String condition = summary.condition.isEmpty()
            ? String("UNAVAILABLE")
            : summary.condition;
        lv_obj_set_style_text_font(
            dailyConditionLabels[index],
            condition.length() > 36
                ? &lv_font_montserrat_12
                : &lv_font_montserrat_14,
            LV_PART_MAIN);
        lv_label_set_text(dailyConditionLabels[index], condition.c_str());
        DashboardIcons::setWeatherCondition(dailyForecastIcons[index], condition);
    }

    String alertText = forecast.alertCount == 0
        ? "NO ACTIVE ALERTS"
        : String("ALERT: ") + forecast.primaryAlert;
    if (alertText.length() > 31) alertText = alertText.substring(0, 28) + "...";
    lv_label_set_text(alertLabel, alertText.c_str());
    updateHourlyForecast();
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
