#include "DashboardScreen.h"
#include <WiFi.h>
#include "../services/NetworkUpdateState.h"
#include "../ui/DashboardIcons.h"
#include "../ui/Theme.h"

namespace
{
    constexpr int16_t PANEL_GAP = 8;
    constexpr int16_t PANEL_TOP = Theme::CONTENT_TOP;
    constexpr int16_t PANEL_HEIGHT = 179;

    constexpr int16_t LEFT_PANEL_X = 8;
    constexpr int16_t CENTER_PANEL_X = 268;
    constexpr int16_t RIGHT_PANEL_X = 528;

    constexpr int16_t LEFT_PANEL_WIDTH = 252;
    constexpr int16_t CENTER_PANEL_WIDTH = 252;
    constexpr int16_t RIGHT_PANEL_WIDTH = 264;

    void mixSignature(uint32_t& signature, uint32_t value)
    {
        signature ^= value;
        signature *= 16777619UL;
    }

    void mixSignature(uint32_t& signature, const String& value)
    {
        for (size_t index = 0; index < value.length(); ++index)
            mixSignature(signature, static_cast<uint8_t>(value[index]));
    }

}

void DashboardScreen::begin(
    ClockService& clockServiceReference,
    WeatherService& weatherServiceReference,
    AircraftService& aircraftServiceReference,
    SatelliteService& satelliteServiceReference,
    SolarService& solarServiceReference,
    LiveSpotsService& liveSpotsServiceReference,
    PotaService& potaServiceReference)
{
    if (screen != nullptr)
    {
        return;
    }

    clockService = &clockServiceReference;
    weatherService = &weatherServiceReference;
    aircraftService = &aircraftServiceReference;
    satelliteService = &satelliteServiceReference;
    solarService = &solarServiceReference;
    liveSpotsService = &liveSpotsServiceReference;
    potaService = &potaServiceReference;

    screen = lv_obj_create(nullptr);

    Theme::configureScreen(screen);

    headerBar.create(
        screen,
        clockServiceReference,
        Theme::SCREEN_WIDTH,
        Theme::HEADER_HEIGHT);
    headerBar.setSettingsCallback(
        [this]()
        {
            if (navigationCallback != nullptr)
            {
                navigationCallback(Page::Settings);
            }
        });
    headerBar.setNavigationCallback(
        [this](Page page)
        {
            if (navigationCallback != nullptr) navigationCallback(page);
        });

    /*
     * Top-left: Weather
     */
    weatherPanel.create(
        screen,
        weatherServiceReference,
        LEFT_PANEL_X,
        PANEL_TOP,
        LEFT_PANEL_WIDTH,
        PANEL_HEIGHT);

    weatherPanel.setClickCallback(
        [this]()
        {
            if (navigationCallback != nullptr)
            {
                navigationCallback(Page::Weather);
            }
        });

    /*
     * Top-right: Live spots
     */
    liveSpotsPanel.create(
        screen,
        liveSpotsServiceReference,
        RIGHT_PANEL_X,
        PANEL_TOP,
        RIGHT_PANEL_WIDTH,
        PANEL_HEIGHT);
    liveSpotsPanel.setClickCallback(
        [this]()
        {
            if (navigationCallback != nullptr) navigationCallback(Page::LiveSpots);
        });

    /*
     * Bottom-center: Satellites
     */
    satellitePanel.create(
        screen,
        satelliteServiceReference,
        CENTER_PANEL_X,
        PANEL_TOP + PANEL_HEIGHT + PANEL_GAP,
        CENTER_PANEL_WIDTH,
        PANEL_HEIGHT);

    satellitePanel.setClickCallback(
        [this]()
        {
            if (navigationCallback != nullptr) navigationCallback(Page::Satellite);
        });

    /*
     * Top-center: Aircraft
     */
    aircraftPanel.create(
        screen,
        aircraftServiceReference,
        CENTER_PANEL_X,
        PANEL_TOP,
        CENTER_PANEL_WIDTH,
        PANEL_HEIGHT);

    aircraftPanel.setClickCallback(
        [this]()
        {
            if (navigationCallback != nullptr) navigationCallback(Page::Aircraft);
        });

    /*
     * Bottom-left: Solar
     */
    solarPanel.create(
        screen,
        solarServiceReference,
        LEFT_PANEL_X,
        PANEL_TOP + PANEL_HEIGHT + PANEL_GAP,
        LEFT_PANEL_WIDTH,
        PANEL_HEIGHT);
    solarPanel.setClickCallback(
        [this]()
        {
            if (navigationCallback != nullptr) navigationCallback(Page::Solar);
        });

    /*
     * Bottom-right: Nearby POTA activators
     */
    potaPanel.create(
        screen,
        potaServiceReference,
        RIGHT_PANEL_X,
        PANEL_TOP + PANEL_HEIGHT + PANEL_GAP,
        RIGHT_PANEL_WIDTH,
        PANEL_HEIGHT);
    potaPanel.setClickCallback(
        [this]()
        {
            if (navigationCallback != nullptr) navigationCallback(Page::Pota);
        });

    update();

    updateTimer = lv_timer_create(
        updateTimerCallback,
        1000,
        this);
}

void DashboardScreen::show()
{
    if (screen == nullptr)
    {
        return;
    }

    // The dashboard is substantially more complex than a detail page. Updating
    // all six panels after loading caused two visible full-screen redraw waves
    // on the single-buffered RGB panel. Settle content and layout while this
    // screen is still hidden, then expose one complete frame.
    update();
    lv_obj_update_layout(screen);
    DashboardIcons::warmCache(
        weatherService != nullptr && weatherService->isValid()
            ? weatherService->getCurrentWeather().condition
            : String());
    lv_scr_load(screen);
}

void DashboardScreen::update()
{
    /*
     * Allow the weather service to advance through its states:
     * waiting for Wi-Fi, resolving the station and downloading data.
     */
    headerBar.update();
    if (NetworkUpdateState::isBusy()) return;

    // LVGL invalidates labels and images even when the new value is identical.
    // The dashboard has six panels, so rewriting them every second caused a
    // broad redraw that the partial RGB buffer exposed as horizontal shimmer.
    const uint32_t signature = calculatePanelSignature();
    if (signature == renderedPanelSignature) return;
    renderedPanelSignature = signature;

    weatherPanel.update();
    aircraftPanel.update();
    satellitePanel.update();
    solarPanel.update();
    liveSpotsPanel.update();
    potaPanel.update();
}

uint32_t DashboardScreen::calculatePanelSignature() const
{
    uint32_t signature = 2166136261UL;
    mixSignature(signature, static_cast<uint32_t>(WiFi.status()));

    if (weatherService != nullptr)
    {
        mixSignature(signature, weatherService->isValid());
        mixSignature(signature, weatherService->isUpdating());
        mixSignature(signature, weatherService->getDataRevision());
        if (!weatherService->isValid())
            mixSignature(signature, weatherService->getLastError());
    }

    if (aircraftService != nullptr)
    {
        mixSignature(signature, aircraftService->isValid());
        mixSignature(signature, aircraftService->isUpdating());
        mixSignature(signature, aircraftService->getAircraftCount());
        mixSignature(signature, aircraftService->getLastUpdateTime());
        if (!aircraftService->isValid())
            mixSignature(signature, aircraftService->getLastError());
    }

    if (satelliteService != nullptr)
    {
        mixSignature(signature, satelliteService->isValid());
        mixSignature(signature, satelliteService->isUpdating());
        if (!satelliteService->isValid())
        {
            mixSignature(signature, satelliteService->getLastError());
        }
        else
        {
            const SatelliteData* pass = satelliteService->getNextPass();
            if (pass != nullptr)
            {
                mixSignature(signature, pass->name);
                mixSignature(signature, pass->aosTime);
            }
            for (uint8_t index = 0;
                 index < SatelliteService::SATELLITE_COUNT; ++index)
            {
                mixSignature(
                    signature,
                    satelliteService->getSatellite(index).visible ? 1U : 0U);
            }
        }
    }

    if (solarService != nullptr)
    {
        mixSignature(signature, solarService->isValid());
        mixSignature(signature, solarService->isUpdating());
        if (solarService->isValid())
            mixSignature(signature, solarService->getData().updatedAt);
        else
            mixSignature(signature, solarService->getLastError());
    }

    if (liveSpotsService != nullptr)
    {
        mixSignature(signature, liveSpotsService->isValid());
        mixSignature(signature, liveSpotsService->isUpdating());
        mixSignature(signature, liveSpotsService->getTotalSpotCount());
        if (!liveSpotsService->isValid())
            mixSignature(signature, liveSpotsService->getLastError());
    }

    if (potaService != nullptr)
    {
        mixSignature(signature, potaService->isValid());
        mixSignature(signature, potaService->isUpdating());
        mixSignature(signature, potaService->getSpotCount());
        if (!potaService->isValid())
            mixSignature(signature, potaService->getLastError());
    }

    return signature;
}

void DashboardScreen::updateTimerCallback(lv_timer_t* timer)
{
    DashboardScreen* dashboard =
        static_cast<DashboardScreen*>(
            timer->user_data);

    if (dashboard == nullptr)
    {
        return;
    }

    dashboard->update();
}

void DashboardScreen::setNavigationCallback(
    NavigationCallback callback)
{
    navigationCallback = callback;
}
