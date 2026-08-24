#include "WiFiService.h"
#include "NetworkUpdateState.h"
#include "SettingsService.h"
#include <esp_err.h>
#include <esp_wifi.h>

WiFiService* WiFiService::eventOwner = nullptr;

void WiFiService::eventHandler(WiFiEvent_t event, WiFiEventInfo_t info)
{
    if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED)
    {
        Serial.print("[WiFi] Disconnected, reason ");
        Serial.println(info.wifi_sta_disconnected.reason);
        if (eventOwner != nullptr)
        {
            eventOwner->connectionReported = false;
            if (eventOwner->credentialsPending &&
                (info.wifi_sta_disconnected.reason == WIFI_REASON_AUTH_FAIL ||
                 info.wifi_sta_disconnected.reason == WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT))
                eventOwner->candidateAuthenticationFailed = true;
        }
    }
    else if (event == ARDUINO_EVENT_WIFI_STA_LOST_IP)
    {
        Serial.println("[WiFi] DHCP address lost");
        if (eventOwner != nullptr) eventOwner->connectionReported = false;
    }
    else if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP)
    {
        Serial.print("[WiFi] DHCP address received: ");
        Serial.println(IPAddress(info.got_ip.ip_info.ip.addr));
    }
}

void WiFiService::startConnection(bool eraseDriverCredentials)
{
    WiFi.disconnect(true, eraseDriverCredentials);
    delay(150);
    WiFi.mode(WIFI_STA);
    WiFi.persistent(false);
    // MissionControl owns retries. The ESP32 driver's automatic reconnect can
    // otherwise keep retrying stale credentials while a scan or SAVE is trying
    // to move the station to a different network.
    WiFi.setAutoReconnect(false);
    // This is a continuously powered display. Disabling station sleep avoids
    // AP compatibility problems and idle disconnects at negligible system cost.
    WiFi.setSleep(false);
    if (!hostname.isEmpty()) WiFi.setHostname(hostname.c_str());
    WiFi.begin(ssid.c_str(), password.c_str());
    connectionStartMillis = millis();
    lastRetryMillis = connectionStartMillis;
    connectionReported = false;
    Serial.print("Connecting to WiFi network: ");
    Serial.println(ssid);
}

void WiFiService::requestReconnect(const char* reason)
{
    Serial.print("[WiFi] Reconnect requested: ");
    Serial.println(reason);
    NetworkUpdateState::setPaused(true);
    reconnectState = ReconnectState::WaitingForWorker;
}

void WiFiService::begin(const AppSettings& settings)
{
    Serial.println();
    Serial.println("Starting WiFi service...");
    ssid = settings.wifiSsid;
    password = settings.wifiPassword;
    hostname = settings.hostname;
    verifiedSsid = ssid;
    verifiedPassword = password;
    verifiedHostname = hostname;
    if (ssid.isEmpty() || password.isEmpty())
    {
        Serial.println("WiFi credentials not configured.");
        return;
    }

    if (!eventHandlerRegistered)
    {
        eventOwner = this;
        WiFi.onEvent(eventHandler);
        eventHandlerRegistered = true;
    }

    if (started)
    {
        requestReconnect("new settings saved");
        return;
    }

    started = true;
    startConnection(true);
}

void WiFiService::applySettings(
    const AppSettings& settings,
    SettingsService& settingsStore)
{
    ssid = settings.wifiSsid;
    password = settings.wifiPassword;
    hostname = settings.hostname;
    pendingSettings = settings;
    pendingSettingsStore = &settingsStore;
    credentialsPending = true;
    credentialsRejected = false;
    candidateAuthenticationFailed = false;
    requestReconnect("testing new settings");
}

void WiFiService::update()
{
    if (ssid.isEmpty() || password.isEmpty()) return;

    const uint32_t now = millis();
    if (reconnectState == ReconnectState::WaitingForWorker)
    {
        if (NetworkUpdateState::isBusy()) return;
        Serial.println("[WiFi] Network worker idle; switching networks");
        WiFi.setAutoReconnect(false);
        // Reset the station and WPA state without deinitializing the driver.
        // WIFI_OFF would release the preallocated RX buffers, which cannot be
        // allocated again after the UI has consumed internal RAM.
        const esp_err_t disconnectResult = esp_wifi_disconnect();
        const esp_err_t stopResult = esp_wifi_stop();
        Serial.print("[WiFi] Radio reset: disconnect=");
        Serial.print(esp_err_to_name(disconnectResult));
        Serial.print(", stop=");
        Serial.println(esp_err_to_name(stopResult));
        radioSettleUntilMillis = now + 300;
        reconnectState = ReconnectState::RadioSettling;
        return;
    }
    if (reconnectState == ReconnectState::RadioSettling)
    {
        if (static_cast<int32_t>(now - radioSettleUntilMillis) < 0) return;
        const esp_err_t startResult = esp_wifi_start();
        Serial.print("[WiFi] Radio restart: ");
        Serial.println(esp_err_to_name(startResult));
        if (startResult != ESP_OK)
        {
            radioSettleUntilMillis = now + 1000;
            return;
        }
        WiFi.persistent(false);
        WiFi.setAutoReconnect(false);
        WiFi.setSleep(false);
        if (!hostname.isEmpty()) WiFi.setHostname(hostname.c_str());
        WiFi.begin(ssid.c_str(), password.c_str());
        connectionStartMillis = now;
        lastRetryMillis = now;
        connectionReported = false;
        reconnectState = ReconnectState::Idle;
        NetworkUpdateState::setPaused(false);
        Serial.print("[WiFi] Connecting with saved settings to: ");
        Serial.println(ssid);
        return;
    }

    // A candidate is never allowed to retry forever. Restore the last network
    // that actually obtained an IP address after an explicit authentication
    // rejection or after the bounded connection window expires.
    if (credentialsPending && reconnectState == ReconnectState::Idle &&
        (candidateAuthenticationFailed ||
         now - connectionStartMillis >= CONNECTION_TIMEOUT_MS))
    {
        Serial.println("[WiFi] Candidate credentials rejected; restoring last verified network");
        ssid = verifiedSsid;
        password = verifiedPassword;
        hostname = verifiedHostname;
        credentialsPending = false;
        credentialsRejected = true;
        candidateAuthenticationFailed = false;
        pendingSettingsStore = nullptr;
        requestReconnect("candidate failed; rollback");
        return;
    }

    if (isNetworkReady())
    {
        if (credentialsPending && WiFi.SSID() == ssid)
        {
            if (pendingSettingsStore != nullptr &&
                pendingSettingsStore->save(pendingSettings))
                Serial.println("[WiFi] New credentials verified and saved");
            else
                Serial.println("[WiFi] WARNING: connected, but credentials could not be saved");
            credentialsPending = false;
            verifiedSsid = ssid;
            verifiedPassword = password;
            verifiedHostname = hostname;
            credentialsRejected = false;
            candidateAuthenticationFailed = false;
            pendingSettingsStore = nullptr;
        }
        if (!connectionReported)
        {
            connectionReported = true;
            Serial.println();
            Serial.println("WiFi connected!");
            Serial.print("Network: "); Serial.println(WiFi.SSID());
            Serial.print("Hostname: "); Serial.println(hostname.isEmpty() ? "(not set)" : hostname);
            Serial.print("IP address: "); Serial.println(WiFi.localIP());
            Serial.print("Gateway: "); Serial.println(WiFi.gatewayIP());
            Serial.print("DNS primary: "); Serial.println(WiFi.dnsIP(0));
            Serial.print("Signal strength: "); Serial.print(WiFi.RSSI()); Serial.println(" dBm");
        }
        return;
    }
    connectionReported = false;
    if (now - connectionStartMillis < CONNECTION_TIMEOUT_MS ||
        now - lastRetryMillis < RETRY_INTERVAL_MS) return;
    Serial.println();
    if (WiFi.status() == WL_CONNECTED)
        Serial.println("[WiFi] Associated but DHCP address is missing; recovering...");
    else
        Serial.println("WiFi not connected. Retrying...");
    requestReconnect("connection recovery");
    lastRetryMillis = now;
}

bool WiFiService::isConnected() const { return WiFi.status() == WL_CONNECTED; }

bool WiFiService::isNetworkReady() const
{
    return isConnected() && static_cast<uint32_t>(WiFi.localIP()) != 0;
}

bool WiFiService::wereCredentialsRejected() const
{
    return credentialsRejected;
}

int WiFiService::scanNetworks()
{
    Serial.println("Pausing panel requests for non-disruptive WiFi scan...");
    NetworkUpdateState::setPaused(true);
    const uint32_t workerDeadline = millis() + 12000UL;
    while (NetworkUpdateState::isBusy() &&
           static_cast<int32_t>(millis() - workerDeadline) < 0)
        delay(10);
    if (NetworkUpdateState::isBusy())
    {
        Serial.println("WiFi scan deferred: panel network request is still active");
        NetworkUpdateState::setPaused(false);
        return -3;
    }


    // Opening the browser abandons an unverified candidate. After the scan,
    // resume the last known-good network instead of restarting the failed one.
    if (credentialsPending)
    {
        Serial.println("[WiFi] Cancelling unverified credentials for network scan");
        ssid = verifiedSsid;
        password = verifiedPassword;
        hostname = verifiedHostname;
        credentialsPending = false;
        credentialsRejected = true;
        candidateAuthenticationFailed = false;
        pendingSettingsStore = nullptr;
        reconnectAfterScan = true;
    }

    // Scan in place. ESP32 station mode supports scanning while associated;
    // disconnecting here made the post-UI driver unable to reassociate on this
    // hardware. If already disconnected, finishNetworkScan schedules a clean
    // reconnect after the synchronous scan has released the radio.
    reconnectState = ReconnectState::Idle;
    WiFi.setAutoReconnect(false);
    WiFi.setSleep(false);
    WiFi.scanDelete();
    reconnectAfterScan = reconnectAfterScan || !isNetworkReady();
    const int found = WiFi.scanNetworks(false, true);
    Serial.print("WiFi scan result: "); Serial.println(found);
    return found;
}

void WiFiService::finishNetworkScan()
{
    WiFi.scanDelete();
    NetworkUpdateState::setPaused(false);
    if (reconnectAfterScan)
    {
        reconnectAfterScan = false;
        requestReconnect("resume verified network after scan");
    }
    else
    {
        Serial.println("WiFi scan complete; existing connection retained");
    }
}
