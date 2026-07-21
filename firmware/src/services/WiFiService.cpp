#include "WiFiService.h"


void WiFiService::begin()
{
    Serial.println();
    Serial.println("Connecting to WiFi...");

    if (ssid == "" || password == "")
    {
        Serial.println("WiFi credentials not configured");
        return;
    }

    WiFi.begin(ssid, password);

    int attempts = 0;

    while (WiFi.status() != WL_CONNECTED && attempts < 20)
    {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println();
        Serial.println("WiFi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println();
        Serial.println("WiFi connection failed");
    }
}


bool WiFiService::isConnected()
{
    return WiFi.status() == WL_CONNECTED;
}