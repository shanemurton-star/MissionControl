#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include "../models/AppSettings.h"

class SettingsService;

class WiFiService
{
public:
    void begin(const AppSettings& settings);
    void applySettings(const AppSettings& settings, SettingsService& settingsStore);
    void update();
    bool isConnected() const;
    bool isNetworkReady() const;
    bool wereCredentialsRejected() const;
    int scanNetworks();
    void finishNetworkScan();

private:
    enum class ReconnectState : uint8_t
    {
        Idle,
        WaitingForWorker,
        RadioSettling
    };

    void startConnection(bool eraseDriverCredentials);
    void requestReconnect(const char* reason);
    static void eventHandler(WiFiEvent_t event, WiFiEventInfo_t info);

    String ssid;
    String password;
    String hostname;
    String verifiedSsid;
    String verifiedPassword;
    String verifiedHostname;
    uint32_t connectionStartMillis = 0;
    uint32_t lastRetryMillis = 0;
    bool connectionReported = false;
    bool started = false;
    bool eventHandlerRegistered = false;
    bool reconnectAfterScan = false;
    bool credentialsPending = false;
    bool credentialsRejected = false;
    bool candidateAuthenticationFailed = false;
    AppSettings pendingSettings;
    SettingsService* pendingSettingsStore = nullptr;
    ReconnectState reconnectState = ReconnectState::Idle;
    uint32_t radioSettleUntilMillis = 0;
    static constexpr uint32_t CONNECTION_TIMEOUT_MS = 15000;
    static constexpr uint32_t RETRY_INTERVAL_MS = 15000;
    static WiFiService* eventOwner;
};
