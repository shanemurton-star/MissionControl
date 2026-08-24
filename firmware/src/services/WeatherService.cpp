#include "WeatherService.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>

#include <math.h>

namespace
{
    const char* NWS_USER_AGENT =
        "MissionControl-ESP32/1.0 "
        "(Holly, Michigan; amateur-radio dashboard)";

    const char* NWS_ACCEPT_TYPE =
        "application/geo+json";

    bool isValidNumber(float value)
    {
        return !isnan(value) &&
               !isinf(value);
    }

    float readMeasurementValue(
        JsonVariantConst measurement)
    {
        if (measurement.isNull())
        {
            return NAN;
        }

        JsonVariantConst value =
            measurement["value"];

        if (value.isNull())
        {
            return NAN;
        }

        return value.as<float>();
    }

    float calculateRelativeHumidity(
        float temperatureC,
        float dewPointC)
    {
        if (!isValidNumber(temperatureC) ||
            !isValidNumber(dewPointC))
        {
            return NAN;
        }

        // Magnus approximation for relative humidity from air temperature
        // and dew point. NWS stations occasionally omit relativeHumidity even
        // when both of these measurements are present.
        const float dewExponent =
            (17.625f * dewPointC) / (243.04f + dewPointC);
        const float temperatureExponent =
            (17.625f * temperatureC) / (243.04f + temperatureC);
        const float humidity =
            100.0f * expf(dewExponent - temperatureExponent);

        return constrain(humidity, 0.0f, 100.0f);
    }

    String readUnitCode(
        JsonVariantConst measurement)
    {
        if (measurement.isNull())
        {
            return "";
        }

        const char* unitCode =
            measurement["unitCode"] | "";

        return String(unitCode);
    }
}

void WeatherService::begin(
    const AppSettings& settings)
{
    latitude =
        settings.latitude;

    longitude =
        settings.longitude;

    refreshIntervalMs = 15UL * 60UL * 1000UL;

    currentWeather = WeatherData();
    forecast = ForecastData();

    stationsUrl = "";
    latestObservationUrl = "";
    forecastUrl = "";
    alertsUrl =
        String("https://api.weather.gov/alerts/active?point=") +
        String(latitude, 4) +
        "," +
        String(longitude, 4);
    lastError = "";

    nextActionMs = 0;
    dataRevision = 0;
    updating = false;

    if (WiFi.status() == WL_CONNECTED)
    {
        state = State::ResolvePoint;
    }
    else
    {
        state = State::WaitingForWiFi;
    }
}

void WeatherService::update()
{
    /*
     * Do not start an API request until Wi-Fi is available.
     */
    if (WiFi.status() != WL_CONNECTED)
    {
        updating = false;
        state = State::WaitingForWiFi;
        return;
    }

    switch (state)
    {
        case State::WaitingForWiFi:
        {
            state = State::ResolvePoint;
            nextActionMs = 0;
            break;
        }

        case State::ResolvePoint:
        {
            if (!timeReached(nextActionMs))
            {
                return;
            }

            updating = true;
            resolvePoint();
            break;
        }

        case State::ResolveStation:
        {
            if (!timeReached(nextActionMs))
            {
                return;
            }

            updating = true;
            resolveStation();
            break;
        }

        case State::FetchObservation:
        {
            if (!timeReached(nextActionMs))
            {
                return;
            }

            updating = true;
            fetchObservation();
            break;
        }

        case State::FetchForecast:
        {
            updating = true;
            fetchForecast();
            break;
        }

        case State::FetchAlerts:
        {
            updating = true;
            fetchAlerts();
            break;
        }

        case State::FetchAirQuality:
        {
            updating = true;
            fetchAirQuality();
            break;
        }

        case State::Ready:
        {
            updating = false;

            if (timeReached(nextActionMs))
            {
                /*
                 * The station URL has already been resolved, so routine
                 * refreshes only need the latest observation request.
                 */
                state = State::FetchObservation;
            }

            break;
        }

        case State::RetryDelay:
        {
            updating = false;

            if (timeReached(nextActionMs))
            {
                /*
                 * If we already know the observation URL, retry only the
                 * observation. Otherwise restart point resolution.
                 */
                if (!latestObservationUrl.isEmpty())
                {
                    state = State::FetchObservation;
                }
                else
                {
                    state = State::ResolvePoint;
                }
            }

            break;
        }
    }
}

bool WeatherService::isValid() const
{
    return currentWeather.valid;
}

bool WeatherService::isUpdating() const
{
    return updating;
}

uint32_t WeatherService::getDataRevision() const
{
    return dataRevision;
}

const WeatherData&
WeatherService::getCurrentWeather() const
{
    return currentWeather;
}

const ForecastData&
WeatherService::getForecast() const
{
    return forecast;
}

const String&
WeatherService::getLastError() const
{
    return lastError;
}

String WeatherService::getTemperature() const
{
    if (!isValidNumber(
            currentWeather.temperatureF))
    {
        return "-- F";
    }

    return String(
               currentWeather.temperatureF,
               0) +
           " F";
}

String WeatherService::getHumidity() const
{
    if (!isValidNumber(
            currentWeather.humidityPercent))
    {
        return "--%";
    }

    return String(
               currentWeather.humidityPercent,
               0) +
           "%";
}

String WeatherService::getCondition() const
{
    if (currentWeather.condition.isEmpty())
    {
        return "Weather unavailable";
    }

    return currentWeather.condition;
}

String WeatherService::getWind() const
{
    if (!isValidNumber(
            currentWeather.windSpeedMph))
    {
        return "-- mph";
    }

    String result =
        currentWeather.windDirection;

    if (!result.isEmpty() &&
        result != "--")
    {
        result += " ";
    }

    result += String(
        currentWeather.windSpeedMph,
        0);

    result += " mph";

    if (isValidNumber(
            currentWeather.windGustMph))
    {
        result += " G";
        result += String(
            currentWeather.windGustMph,
            0);
    }

    return result;
}

String WeatherService::getPressure() const
{
    if (!isValidNumber(
            currentWeather.pressureInHg))
    {
        return "--.-- inHg";
    }

    return String(
               currentWeather.pressureInHg,
               2) +
           " inHg";
}

String WeatherService::getDewPoint() const
{
    if (!isValidNumber(
            currentWeather.dewPointF))
    {
        return "-- F";
    }

    return String(
               currentWeather.dewPointF,
               0) +
           " F";
}

String WeatherService::getStationId() const
{
    if (currentWeather.stationId.isEmpty())
    {
        return "--";
    }

    return currentWeather.stationId;
}

String WeatherService::getObservationTime() const
{
    if (currentWeather.observationTime.isEmpty())
    {
        return "--";
    }

    return currentWeather.observationTime;
}

void WeatherService::resolvePoint()
{
    const String pointUrl =
        String("https://api.weather.gov/points/") +
        String(latitude, 4) +
        "," +
        String(longitude, 4);

    String response;

    if (!performRequest(
            pointUrl,
            response))
    {
        scheduleRetry();
        return;
    }

    JsonDocument document;

    const DeserializationError error =
        deserializeJson(
            document,
            response);

    if (error)
    {
        setError(
            String("Point JSON error: ") +
            error.c_str());

        scheduleRetry();
        return;
    }

    const char* stationEndpoint =
        document["properties"]
                ["observationStations"] |
        "";

    const char* forecastEndpoint =
        document["properties"]["forecast"] | "";

    if (stationEndpoint[0] == '\0')
    {
        setError(
            "NWS point response did not include "
            "observationStations");

        scheduleRetry();
        return;
    }

    stationsUrl =
        String(stationEndpoint);

    forecastUrl =
        String(forecastEndpoint);

    lastError = "";
    state = State::ResolveStation;
    nextActionMs = 0;
}

void WeatherService::resolveStation()
{
    if (stationsUrl.isEmpty())
    {
        setError(
            "Observation station URL is empty");

        scheduleRetry();
        return;
    }

    String response;

    if (!performRequest(
            stationsUrl,
            response))
    {
        scheduleRetry();
        return;
    }

    JsonDocument document;

    const DeserializationError error =
        deserializeJson(
            document,
            response);

    if (error)
    {
        setError(
            String("Station JSON error: ") +
            error.c_str());

        scheduleRetry();
        return;
    }

    JsonArrayConst features =
        document["features"].as<JsonArrayConst>();

    if (features.isNull() ||
        features.size() == 0)
    {
        setError(
            "NWS returned no observation stations");

        scheduleRetry();
        return;
    }

    const char* stationUrl =
        features[0]["id"] | "";

    if (stationUrl[0] == '\0')
    {
        setError(
            "NWS station response did not include "
            "a station ID");

        scheduleRetry();
        return;
    }

    const String stationUrlString =
        String(stationUrl);

    latestObservationUrl =
        stationUrlString +
        "/observations/latest";

    currentWeather.stationId =
        extractStationId(
            stationUrlString);

    lastError = "";
    state = State::FetchObservation;
    nextActionMs = 0;
}

void WeatherService::fetchObservation()
{
    if (latestObservationUrl.isEmpty())
    {
        setError(
            "Latest observation URL is empty");

        scheduleRetry();
        return;
    }

    String response;

    if (!performRequest(
            latestObservationUrl,
            response))
    {
        scheduleRetry();
        return;
    }

    JsonDocument document;

    const DeserializationError error =
        deserializeJson(
            document,
            response);

    if (error)
    {
        setError(
            String("Observation JSON error: ") +
            error.c_str());

        scheduleRetry();
        return;
    }

    JsonObjectConst properties =
        document["properties"]
            .as<JsonObjectConst>();

    if (properties.isNull())
    {
        setError(
            "NWS observation properties are missing");

        scheduleRetry();
        return;
    }

    const char* description =
        properties["textDescription"] |
        "Weather unavailable";

    currentWeather.condition =
        String(description);

    const char* icon =
        properties["icon"] | "";

    currentWeather.iconUrl =
        String(icon);

    const char* timestamp =
        properties["timestamp"] | "";

    currentWeather.observationTime =
        String(timestamp);

    /*
     * Temperature
     */
    const float temperatureC =
        readMeasurementValue(
            properties["temperature"]);

    currentWeather.temperatureF =
        isValidNumber(temperatureC)
            ? celsiusToFahrenheit(
                  temperatureC)
            : NAN;

    /*
     * Relative humidity
     */
    currentWeather.humidityPercent =
        readMeasurementValue(
            properties["relativeHumidity"]);

    /*
     * Dew point
     */
    const float dewPointC =
        readMeasurementValue(
            properties["dewpoint"]);

    currentWeather.dewPointF =
        isValidNumber(dewPointC)
            ? celsiusToFahrenheit(
                  dewPointC)
            : NAN;

    if (!isValidNumber(currentWeather.humidityPercent))
    {
        currentWeather.humidityPercent =
            calculateRelativeHumidity(
                temperatureC,
                dewPointC);
    }

    /*
     * Wind direction
     */
    const float windDirectionDegrees =
        readMeasurementValue(
            properties["windDirection"]);

    currentWeather.windDirection =
        isValidNumber(windDirectionDegrees)
            ? degreesToCompass(
                  windDirectionDegrees)
            : "--";

    /*
     * Wind speed
     */
    const JsonVariantConst windSpeed =
        properties["windSpeed"];

    const float windSpeedValue =
        readMeasurementValue(
            windSpeed);

    const String windSpeedUnit =
        readUnitCode(
            windSpeed);

    if (!isValidNumber(windSpeedValue))
    {
        currentWeather.windSpeedMph = NAN;
    }
    else if (
        windSpeedUnit.indexOf(
            "km_h-1") >= 0)
    {
        currentWeather.windSpeedMph =
            kilometersPerHourToMph(
                windSpeedValue);
    }
    else if (
        windSpeedUnit.indexOf(
            "m_s-1") >= 0)
    {
        currentWeather.windSpeedMph =
            metersPerSecondToMph(
                windSpeedValue);
    }
    else
    {
        /*
         * NWS normally supplies wind speed as km/h.
         * Use km/h as the fallback interpretation.
         */
        currentWeather.windSpeedMph =
            kilometersPerHourToMph(
                windSpeedValue);
    }

    /*
     * Wind gust
     */
    const JsonVariantConst windGust =
        properties["windGust"];

    const float windGustValue =
        readMeasurementValue(
            windGust);

    const String windGustUnit =
        readUnitCode(
            windGust);

    if (!isValidNumber(windGustValue))
    {
        currentWeather.windGustMph = NAN;
    }
    else if (
        windGustUnit.indexOf(
            "m_s-1") >= 0)
    {
        currentWeather.windGustMph =
            metersPerSecondToMph(
                windGustValue);
    }
    else
    {
        currentWeather.windGustMph =
            kilometersPerHourToMph(
                windGustValue);
    }

    /*
     * Barometric pressure
     */
    const float pressurePa =
        readMeasurementValue(
            properties["barometricPressure"]);

    currentWeather.pressureInHg =
        isValidNumber(pressurePa)
            ? pascalsToInHg(
                  pressurePa)
            : NAN;

    currentWeather.valid = true;

    currentWeather.lastSuccessfulUpdateMs =
        millis();

    lastError = "";
    updating = false;

    state = State::FetchForecast;
    nextActionMs = 0;

    Serial.println(
        "[WeatherService] Weather updated");

    Serial.print(
        "[WeatherService] Station: ");

    Serial.println(
        currentWeather.stationId);

    Serial.print(
        "[WeatherService] Condition: ");

    Serial.println(
        currentWeather.condition);

    Serial.print(
        "[WeatherService] Temperature: ");

    Serial.println(
        getTemperature());

    Serial.print(
        "[WeatherService] Wind: ");

    Serial.println(
        getWind());
}

void WeatherService::fetchForecast()
{
    if (forecastUrl.isEmpty())
    {
        setError("NWS point response did not include a forecast URL");
        scheduleRetry();
        return;
    }

    String response;

    if (!performRequest(forecastUrl, response))
    {
        scheduleRetry();
        return;
    }

    JsonDocument document;
    const DeserializationError error =
        deserializeJson(document, response);

    if (error)
    {
        setError(
            String("Forecast JSON error: ") +
            error.c_str());
        scheduleRetry();
        return;
    }

    JsonArrayConst periods =
        document["properties"]["periods"]
            .as<JsonArrayConst>();

    if (periods.isNull() || periods.size() == 0)
    {
        setError("NWS returned no forecast periods");
        scheduleRetry();
        return;
    }

    ForecastData newForecast;

    for (JsonObjectConst period : periods)
    {
        const bool daytime =
            period["isDaytime"] | false;
        const int temperature =
            period["temperature"] | 0;

        if (daytime && !newForecast.hasHigh)
        {
            newForecast.highF = temperature;
            newForecast.hasHigh = true;
        }
        else if (!daytime && !newForecast.hasLow)
        {
            newForecast.lowF = temperature;
            newForecast.hasLow = true;
        }

        if (newForecast.periodCount <
            ForecastData::MAX_PERIODS)
        {
            ForecastPeriod& destination =
                newForecast.periods[
                    newForecast.periodCount++];

            destination.name =
                String(period["name"] | "--");
            destination.shortForecast =
                String(period["shortForecast"] | "--");
            destination.temperatureF = temperature;
            destination.daytime = daytime;
        }

        if (newForecast.hasHigh &&
            newForecast.hasLow &&
            newForecast.periodCount >=
                ForecastData::MAX_PERIODS)
        {
            break;
        }
    }

    newForecast.valid =
        newForecast.periodCount > 0;

    newForecast.alertCount = forecast.alertCount;
    newForecast.primaryAlert = forecast.primaryAlert;
    forecast = newForecast;

    lastError = "";
    state = State::FetchAlerts;
}

void WeatherService::fetchAlerts()
{
    String response;

    if (!performRequest(alertsUrl, response))
    {
        scheduleRetry();
        return;
    }

    JsonDocument document;
    const DeserializationError error =
        deserializeJson(document, response);

    if (error)
    {
        setError(
            String("Alerts JSON error: ") +
            error.c_str());
        scheduleRetry();
        return;
    }

    JsonArrayConst features =
        document["features"].as<JsonArrayConst>();

    forecast.alertCount =
        features.isNull() ? 0 : features.size();
    forecast.primaryAlert = "";

    if (!features.isNull() && features.size() > 0)
    {
        const char* event =
            features[0]["properties"]["event"] | "";
        forecast.primaryAlert = String(event);
    }

    lastError = "";
    state = State::FetchAirQuality;
}

void WeatherService::fetchAirQuality()
{
    const String url =
        String("https://air-quality-api.open-meteo.com/v1/air-quality?latitude=") +
        String(latitude, 4) + "&longitude=" + String(longitude, 4) +
        "&current=us_aqi,pm2_5,pm10";

    String response;
    currentWeather.airQualityValid = false;
    if (performRequest(url, response))
    {
        JsonDocument document;
        const DeserializationError error = deserializeJson(document, response);
        if (!error)
        {
            JsonObjectConst current = document["current"].as<JsonObjectConst>();
            if (!current.isNull() && !current["us_aqi"].isNull())
            {
                currentWeather.usAqi = static_cast<int16_t>(current["us_aqi"].as<float>() + 0.5f);
                currentWeather.pm25 = current["pm2_5"] | NAN;
                currentWeather.pm10 = current["pm10"] | NAN;
                currentWeather.airQualityValid = true;
            }
        }
    }

    // Some NWS observation stations omit humidity, dew point, or wind. Obtain
    // only missing current values from Open-Meteo; never replace measurements
    // that the station did report.
    const bool needsWeatherFallback =
        !isValidNumber(currentWeather.humidityPercent) ||
        !isValidNumber(currentWeather.dewPointF) ||
        !isValidNumber(currentWeather.windSpeedMph) ||
        currentWeather.windDirection == "--";

    if (needsWeatherFallback)
    {
        const String humidityUrl =
            String("https://api.open-meteo.com/v1/forecast?latitude=") +
            String(latitude, 4) + "&longitude=" + String(longitude, 4) +
            "&current=relative_humidity_2m,dew_point_2m,wind_speed_10m,"
            "wind_direction_10m,wind_gusts_10m&temperature_unit=fahrenheit"
            "&wind_speed_unit=mph&forecast_days=1";

        String humidityResponse;
        if (performRequest(humidityUrl, humidityResponse))
        {
            JsonDocument humidityDocument;
            const DeserializationError humidityError =
                deserializeJson(humidityDocument, humidityResponse);
            if (!humidityError)
            {
                JsonObjectConst current =
                    humidityDocument["current"].as<JsonObjectConst>();

                if (!isValidNumber(currentWeather.humidityPercent) &&
                    !current["relative_humidity_2m"].isNull())
                {
                    currentWeather.humidityPercent =
                        current["relative_humidity_2m"].as<float>();
                }
                if (!isValidNumber(currentWeather.dewPointF) &&
                    !current["dew_point_2m"].isNull())
                {
                    currentWeather.dewPointF =
                        current["dew_point_2m"].as<float>();
                }
                if (!isValidNumber(currentWeather.windSpeedMph) &&
                    !current["wind_speed_10m"].isNull())
                {
                    currentWeather.windSpeedMph =
                        current["wind_speed_10m"].as<float>();
                }
                if (!isValidNumber(currentWeather.windGustMph) &&
                    !current["wind_gusts_10m"].isNull())
                {
                    currentWeather.windGustMph =
                        current["wind_gusts_10m"].as<float>();
                }
                if (currentWeather.windDirection == "--" &&
                    !current["wind_direction_10m"].isNull())
                {
                    currentWeather.windDirection = degreesToCompass(
                        current["wind_direction_10m"].as<float>());
                }

                Serial.println("[WeatherService] Missing observations supplemented by Open-Meteo");
            }
        }
    }

    // Air quality supplements the NWS feed; its failure must not invalidate
    // otherwise current observations, forecasts, or alerts.
    if (!currentWeather.airQualityValid)
        Serial.println("[WeatherService] Air quality unavailable");
    else
    {
        Serial.print("[WeatherService] US AQI: ");
        Serial.println(currentWeather.usAqi);
    }

    lastError = "";
    ++dataRevision;
    updating = false;
    state = State::Ready;
    nextActionMs = millis() + refreshIntervalMs;
}

bool WeatherService::performRequest(
    const String& url,
    String& response)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        setError(
            "Wi-Fi is not connected");

        return false;
    }

    HTTPClient http;

    http.setConnectTimeout(
        REQUEST_TIMEOUT_MS);

    http.setTimeout(
        REQUEST_TIMEOUT_MS);

    if (!http.begin(url))
    {
        setError(
            "Unable to initialize HTTP request");

        return false;
    }

    http.addHeader(
        "User-Agent",
        NWS_USER_AGENT);

    http.addHeader(
        "Accept",
        NWS_ACCEPT_TYPE);

    Serial.print(
        "[WeatherService] GET ");

    Serial.println(url);

    const int responseCode =
        http.GET();

    if (responseCode <= 0)
    {
        setError(
            String("HTTP request failed: ") +
            http.errorToString(
                responseCode));

        http.end();
        return false;
    }

    if (responseCode < 200 ||
        responseCode >= 300)
    {
        setError(
            String("NWS returned HTTP ") +
            String(responseCode));

        http.end();
        return false;
    }

    response =
        http.getString();

    http.end();

    if (response.isEmpty())
    {
        setError(
            "NWS returned an empty response");

        return false;
    }

    return true;
}

void WeatherService::setError(
    const String& message)
{
    lastError = message;
    updating = false;

    Serial.print(
        "[WeatherService] Error: ");

    Serial.println(message);
}

void WeatherService::scheduleRetry()
{
    updating = false;
    state = State::RetryDelay;

    nextActionMs =
        millis() +
        RETRY_INTERVAL_MS;
}

float WeatherService::celsiusToFahrenheit(
    float celsius)
{
    return (
               celsius *
               9.0F /
               5.0F) +
           32.0F;
}

float WeatherService::kilometersPerHourToMph(
    float kilometersPerHour)
{
    return kilometersPerHour *
           0.621371F;
}

float WeatherService::metersPerSecondToMph(
    float metersPerSecond)
{
    return metersPerSecond *
           2.23694F;
}

float WeatherService::pascalsToInHg(
    float pascals)
{
    return pascals *
           0.000295300F;
}

String WeatherService::degreesToCompass(
    float degrees)
{
    static const char* directions[] =
    {
        "N",
        "NNE",
        "NE",
        "ENE",
        "E",
        "ESE",
        "SE",
        "SSE",
        "S",
        "SSW",
        "SW",
        "WSW",
        "W",
        "WNW",
        "NW",
        "NNW"
    };

    while (degrees < 0.0F)
    {
        degrees += 360.0F;
    }

    while (degrees >= 360.0F)
    {
        degrees -= 360.0F;
    }

    const int index =
        static_cast<int>(
            (degrees + 11.25F) /
            22.5F) %
        16;

    return String(
        directions[index]);
}

String WeatherService::extractStationId(
    const String& stationUrl)
{
    const int finalSlash =
        stationUrl.lastIndexOf('/');

    if (finalSlash < 0 ||
        finalSlash >=
            stationUrl.length() - 1)
    {
        return stationUrl;
    }

    return stationUrl.substring(
        finalSlash + 1);
}

bool WeatherService::timeReached(
    unsigned long targetTime)
{
    if (targetTime == 0)
    {
        return true;
    }

    return static_cast<long>(
               millis() -
               targetTime) >= 0;
}
