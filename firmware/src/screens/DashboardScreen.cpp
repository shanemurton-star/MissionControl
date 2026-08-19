#include "DashboardScreen.h"

namespace
{
    constexpr int16_t SCREEN_WIDTH = 800;
    constexpr int16_t SCREEN_HEIGHT = 480;

    constexpr int16_t HEADER_HEIGHT = 64;
    constexpr int16_t PANEL_GAP = 8;
    constexpr int16_t PANEL_TOP = 72;
    constexpr int16_t PANEL_HEIGHT = 179;

    constexpr int16_t LEFT_PANEL_X = 8;
    constexpr int16_t CENTER_PANEL_X = 268;
    constexpr int16_t RIGHT_PANEL_X = 528;

    constexpr int16_t LEFT_PANEL_WIDTH = 252;
    constexpr int16_t CENTER_PANEL_WIDTH = 252;
    constexpr int16_t RIGHT_PANEL_WIDTH = 264;

    lv_color_t colorFromHex(uint32_t value)
    {
        return lv_color_hex(value);
    }

    void configureScreen(lv_obj_t* object)
    {
        lv_obj_set_style_bg_color(
            object,
            colorFromHex(0x11191D),
            LV_PART_MAIN);

        lv_obj_set_style_bg_opa(
            object,
            LV_OPA_COVER,
            LV_PART_MAIN);

        lv_obj_clear_flag(
            object,
            LV_OBJ_FLAG_SCROLLABLE);
    }

    void configurePanel(lv_obj_t* panel)
    {
        lv_obj_set_style_bg_color(
            panel,
            colorFromHex(0x02080C),
            LV_PART_MAIN);

        lv_obj_set_style_bg_opa(
            panel,
            LV_OPA_COVER,
            LV_PART_MAIN);

        lv_obj_set_style_border_color(
            panel,
            colorFromHex(0x1D7891),
            LV_PART_MAIN);

        lv_obj_set_style_border_width(
            panel,
            1,
            LV_PART_MAIN);

        lv_obj_set_style_radius(
            panel,
            8,
            LV_PART_MAIN);

        lv_obj_set_style_pad_all(
            panel,
            10,
            LV_PART_MAIN);

        lv_obj_clear_flag(
            panel,
            LV_OBJ_FLAG_SCROLLABLE);
    }

    lv_obj_t* createLabel(
        lv_obj_t* parent,
        const char* text,
        uint32_t color)
    {
        lv_obj_t* label = lv_label_create(parent);

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

    lv_obj_t* createPanel(
        lv_obj_t* parent,
        int16_t x,
        int16_t y,
        int16_t width,
        int16_t height,
        const char* title)
    {
        lv_obj_t* panel = lv_obj_create(parent);

        lv_obj_set_pos(
            panel,
            x,
            y);

        lv_obj_set_size(
            panel,
            width,
            height);

        configurePanel(panel);

        lv_obj_t* titleLabel = createLabel(
            panel,
            title,
            0x32C7E8);

        lv_obj_align(
            titleLabel,
            LV_ALIGN_TOP_LEFT,
            0,
            0);

        return panel;
    }

    void createSectionDivider(
        lv_obj_t* parent,
        int16_t y,
        int16_t width)
    {
        lv_obj_t* divider = lv_obj_create(parent);

        lv_obj_set_pos(
            divider,
            0,
            y);

        lv_obj_set_size(
            divider,
            width,
            1);

        lv_obj_set_style_bg_color(
            divider,
            colorFromHex(0x1D7891),
            LV_PART_MAIN);

        lv_obj_set_style_bg_opa(
            divider,
            LV_OPA_COVER,
            LV_PART_MAIN);

        lv_obj_set_style_border_width(
            divider,
            0,
            LV_PART_MAIN);

        lv_obj_clear_flag(
            divider,
            LV_OBJ_FLAG_SCROLLABLE);
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

    screen = lv_obj_create(nullptr);

    lv_obj_set_size(
        screen,
        SCREEN_WIDTH,
        SCREEN_HEIGHT);

    configureScreen(screen);

    headerBar.create(
        screen,
        clockServiceReference,
        SCREEN_WIDTH,
        HEADER_HEIGHT);
    headerBar.setSettingsCallback(
        [this]()
        {
            if (navigationCallback != nullptr)
            {
                navigationCallback(Page::Settings);
            }
        });

    createSectionDivider(
        screen,
        HEADER_HEIGHT,
        SCREEN_WIDTH);

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

    lv_scr_load(screen);
    update();
}

void DashboardScreen::update()
{
    /*
     * Allow the weather service to advance through its states:
     * waiting for Wi-Fi, resolving the station and downloading data.
     */
    weatherService->update();

    headerBar.update();
    weatherPanel.update();
    aircraftPanel.update();
    satellitePanel.update();
    solarPanel.update();
    liveSpotsPanel.update();
    potaPanel.update();
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
