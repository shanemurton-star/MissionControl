#include <Arduino.h>
#include <esp_heap_caps.h>
#include <WiFiClient.h>
#include <lwip/dns.h>

#include "config/Version.h"
#include "hardware/DisplayService.h"
#include "services/SettingsService.h"
#include "services/WeatherService.h"
#include "services/WiFiService.h"
#include "services/ClockService.h"
#include "services/AircraftService.h"
#include "services/SatelliteService.h"
#include "services/SolarService.h"
#include "services/LiveSpotsService.h"
#include "services/PotaService.h"
#include "services/NetworkUpdateState.h"

SettingsService settingsService;
DisplayService displayService;
WiFiService wifiService;
ClockService clockService;
WeatherService weatherService;
AircraftService aircraftService;
SatelliteService satelliteService;
SolarService solarService;
LiveSpotsService liveSpotsService;
PotaService potaService;

namespace
{
    TaskHandle_t networkUpdateTaskHandle = nullptr;
    StaticTask_t networkUpdateTaskBuffer;
    StackType_t networkUpdateTaskStack[6144];

    void setDnsServer(uint8_t index, const IPAddress& address)
    {
        ip_addr_t server;
        IP_ADDR4(
            &server,
            address[0], address[1], address[2], address[3]);
        dns_setserver(index, &server);
    }

    void checkPostDisplayNetworking()
    {
        if (!wifiService.isNetworkReady()) return;

        Serial.println("Post-display network check...");
        WiFiClient routeClient;
        const bool rawTcpReady = routeClient.connect(
            IPAddress(1, 1, 1, 1), 443, 5000);
        routeClient.stop();
        Serial.println(rawTcpReady
            ? "PASS: post-display raw TCP reached 1.1.1.1:443"
            : "FAIL: post-display raw TCP could not reach 1.1.1.1:443");

        if (!rawTcpReady) return;

        const IPAddress dhcpDns = WiFi.dnsIP(0);
        setDnsServer(0, IPAddress(1, 1, 1, 1));
        setDnsServer(1, IPAddress(8, 8, 8, 8));
        Serial.println("Testing public DNS after display startup...");
        IPAddress resolved;
        if (WiFi.hostByName("api.zippopotam.us", resolved) == 1)
        {
            Serial.print("PASS: public DNS api.zippopotam.us -> ");
            Serial.println(resolved);
            Serial.println("Using DNS primary 1.1.1.1, backup 8.8.8.8");
            return;
        }

        setDnsServer(0, dhcpDns);
        setDnsServer(1, IPAddress(0, 0, 0, 0));
        Serial.println("FAIL: public DNS unavailable; restored DHCP DNS");
    }

    void networkUpdateTask(void*)
    {
        uint8_t serviceSlot = 0;

        for (;;)
        {
            if (!NetworkUpdateState::isPaused() &&
                wifiService.isNetworkReady() && clockService.isSynchronized())
            {
                NetworkUpdateState::setBusy(true);
                if (NetworkUpdateState::isPaused())
                {
                    NetworkUpdateState::setBusy(false);
                    vTaskDelay(pdMS_TO_TICKS(50));
                    continue;
                }
                switch (serviceSlot)
                {
                    case 0: weatherService.update(); break;
                    case 1: aircraftService.update(); break;
                    case 2: satelliteService.update(); break;
                    case 3: solarService.update(); break;
                    case 4: liveSpotsService.update(); break;
                    case 5: potaService.update(); break;
                }
                NetworkUpdateState::setBusy(false);
                serviceSlot = (serviceSlot + 1) % 6;
            }

            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}

void setup()
{
    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println("==============================");
    Serial.println("Mission Control starting...");
    Serial.println("==============================");
    Serial.print("Firmware build: ");
    Serial.print(__DATE__);
    Serial.print(" ");
    Serial.println(__TIME__);
    Serial.print("Firmware version: ");
    Serial.println(Version::FIRMWARE);
    Serial.println("Network revision: WIFI-32 / boot-tested WiFi settings");

    /*
     * Load all persistent settings before starting any
     * service that depends on them.
     */
    if (!settingsService.begin())
    {
        Serial.println(
            "ERROR: Settings service failed to initialize.");
    }

    /*
     * Start Wi-Fi using the loaded settings.
     */
    wifiService.begin(
        settingsService.get());

    /*
     * Initialize the weather service.
     * It will wait for Wi-Fi before contacting the NWS.
     *
     * Weather will be connected to SettingsService in the
     * next step.
     */
    clockService.begin();

    // The isolated network diagnostic proved that DHCP, numeric NTP, DNS, and
    // HTTPS all work before the display stack is initialized. Give SNTP that
    // same quiet startup window here, before LVGL and the panel screens consume
    // memory and processing time. Both waits are bounded so the UI will still
    // start when the network is unavailable.
    const uint32_t wifiStartupDeadline = millis() + 20000UL;
    while (!wifiService.isNetworkReady() &&
           static_cast<int32_t>(millis() - wifiStartupDeadline) < 0)
    {
        wifiService.update();
        delay(100);
    }

    if (settingsService.hasPendingWiFiSettings())
    {
        if (wifiService.isNetworkReady())
        {
            settingsService.commitPendingWiFiSettings();
        }
        else
        {
            Serial.println("Candidate WiFi failed boot test; restoring previous network...");
            settingsService.discardPendingWiFiSettings();
            delay(500);
            ESP.restart();
        }
    }

    else if (wifiService.isNetworkReady() && settingsService.didWiFiCandidateFail())
    {
        // A previous candidate failed, but the stored fallback has now proved
        // that it can connect. Do not let that historical warning mask ZIP,
        // callsign, grid, or other Settings feedback indefinitely.
        settingsService.clearWiFiCandidateFailure();
    }

    if (wifiService.isNetworkReady())
    {
        const uint32_t timeStartupDeadline = millis() + 15000UL;
        while (!clockService.isSynchronized() &&
               static_cast<int32_t>(millis() - timeStartupDeadline) < 0)
        {
            clockService.update();
            delay(100);
        }

        if (!clockService.isSynchronized())
        {
            Serial.println("Initial time synchronization timed out; continuing startup.");
        }
    }
    else
    {
        Serial.println("Initial WiFi connection timed out; continuing startup.");
    }

    weatherService.begin(
        settingsService.get());

    aircraftService.begin(
        settingsService.get());

    satelliteService.begin(
        settingsService.get());
    solarService.begin();
    liveSpotsService.begin(settingsService.get());
    potaService.begin(settingsService.get());

    if (wifiService.isNetworkReady() && clockService.isSynchronized())
    {
        Serial.println("Loading initial panel data before display startup...");

        // DNS and HTTPS were verified to work in the isolated pre-display
        // environment. Complete each service's initial state-machine work in
        // that same window, then let the normal loop handle timed refreshes.
        for (uint8_t step = 0; step < 7; ++step)
        {
            weatherService.update();
            delay(10);
        }

        aircraftService.update();

        // DNS on this display becomes unreliable after the RGB/LVGL hardware
        // stack starts. CelesTrak now has bounded connection/handshake limits,
        // so load its five initial TLEs in the proven pre-display DNS window.
        for (uint8_t satellite = 0;
             satellite < SatelliteService::SATELLITE_COUNT;
             ++satellite)
        {
            satelliteService.update();
            delay(10);
        }

        if (satelliteService.isValid())
        {
            for (uint8_t satellite = 0;
                 satellite < SatelliteService::SATELLITE_COUNT;
                 ++satellite)
            {
                satelliteService.requestRadioData(satellite);
                satelliteService.update();
                delay(10);
            }
        }

        // Load the numeric solar data now, but leave the large JPEG decoder
        // for the dedicated background worker created after display startup.
        solarService.update(false);
        liveSpotsService.update();
        potaService.update();
        Serial.println("Initial panel data loading complete.");
        Serial.print("[Memory] after initial network data internal free=");
        Serial.print(heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
        Serial.print(" largest=");
        Serial.print(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
        Serial.print(" PSRAM free=");
        Serial.println(heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    }

    if (displayService.begin(
        clockService,
        weatherService,
        aircraftService,
        satelliteService,
        solarService,
        liveSpotsService,
        potaService,
        settingsService,
        wifiService))
    {
        Serial.println(
            "Display service initialized.");
    }
    else
    {
        Serial.println(
            "ERROR: Display service failed to initialize.");
    }

    checkPostDisplayNetworking();

    networkUpdateTaskHandle = xTaskCreateStaticPinnedToCore(
        networkUpdateTask,
        "panel-network",
        sizeof(networkUpdateTaskStack),
        nullptr,
        1,
        networkUpdateTaskStack,
        &networkUpdateTaskBuffer,
        0);

    Serial.println(networkUpdateTaskHandle != nullptr
        ? "Background panel network worker started."
        : "ERROR: Unable to start background panel network worker.");
    Serial.print("Free heap after UI and worker: ");
    Serial.println(ESP.getFreeHeap());
    Serial.print("Largest internal heap block: ");
    Serial.println(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
    Serial.print("Total free internal heap: ");
    Serial.println(heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    Serial.print("Free PSRAM after UI and worker: ");
    Serial.println(heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
}

void loop()
{
    // LVGL must run before any synchronous network operation so touch and
    // drawing never wait behind a whole batch of API requests.
    displayService.update();
    wifiService.update();
    clockService.update();

    delay(5);
}
