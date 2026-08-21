#include <Arduino.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WiFi.h>

#include <time.h>

namespace
{
    constexpr uint32_t WIFI_TIMEOUT_MS = 20000;
    constexpr uint32_t TIME_TIMEOUT_MS = 15000;
}

void setup()
{
    Serial.begin(115200);
    delay(500);
    Serial.println();
    Serial.println("=== MissionControl isolated network diagnostic ===");
    Serial.println("No display, LVGL, panels, or application services are running.");

    Preferences preferences;
    if (!preferences.begin("missionctrl", true))
    {
        Serial.println("FAIL: unable to open MissionControl settings");
        return;
    }
    const String ssid = preferences.getString("ssid", "");
    const String password = preferences.getString("wifipass", "");
    preferences.end();

    Serial.print("Saved SSID: ");
    Serial.println(ssid.isEmpty() ? "(empty)" : ssid);
    if (ssid.isEmpty() || password.isEmpty())
    {
        Serial.println("FAIL: saved WiFi credentials are missing");
        return;
    }

    WiFi.disconnect(true, true);
    delay(200);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());

    const uint32_t wifiDeadline = millis() + WIFI_TIMEOUT_MS;
    while (WiFi.status() != WL_CONNECTED &&
           static_cast<int32_t>(millis() - wifiDeadline) < 0)
    {
        delay(100);
    }
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.print("FAIL: WiFi status after 20 seconds = ");
        Serial.println(static_cast<int>(WiFi.status()));
        return;
    }

    Serial.println("PASS: WiFi connected");
    Serial.print("IP: "); Serial.println(WiFi.localIP());
    Serial.print("Gateway: "); Serial.println(WiFi.gatewayIP());
    Serial.print("DNS: "); Serial.println(WiFi.dnsIP(0));
    Serial.print("RSSI: "); Serial.println(WiFi.RSSI());

    WiFiClient routeClient;
    const bool routeReady = routeClient.connect(IPAddress(1, 1, 1, 1), 443, 5000);
    routeClient.stop();
    Serial.println(routeReady
        ? "PASS: raw TCP reached 1.1.1.1:443"
        : "FAIL: raw TCP could not reach 1.1.1.1:443");

    IPAddress resolved;
    const bool dnsReady = WiFi.hostByName("api.weather.gov", resolved) == 1;
    if (dnsReady)
    {
        Serial.print("PASS: DNS api.weather.gov -> ");
        Serial.println(resolved);
    }
    else
    {
        Serial.println("FAIL: DNS could not resolve api.weather.gov");
    }

    configTime(0, 0, "162.159.200.1", "162.159.200.123");
    const uint32_t timeDeadline = millis() + TIME_TIMEOUT_MS;
    while (time(nullptr) < 1704067200 &&
           static_cast<int32_t>(millis() - timeDeadline) < 0)
    {
        delay(100);
    }
    Serial.println(time(nullptr) >= 1704067200
        ? "PASS: numeric NTP synchronized"
        : "FAIL: numeric NTP did not synchronize");

    if (dnsReady)
    {
        HTTPClient http;
        http.setConnectTimeout(5000);
        http.setTimeout(5000);
        const bool started = http.begin("https://api.weather.gov");
        const int code = started ? http.GET() : -999;
        http.end();
        Serial.print("HTTPS api.weather.gov result: ");
        Serial.println(code);
    }

    Serial.println("=== Diagnostic complete ===");
}

void loop()
{
    delay(1000);
}
