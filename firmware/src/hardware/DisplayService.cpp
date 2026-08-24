#include "DisplayService.h"

#include <Arduino.h>
#include <Wire.h>
#include <lvgl.h>
#include <LovyanGFX.hpp>
#include <esp_heap_caps.h>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>

#include "../screens/ScreenManager.h"
#include "../ui/HeaderBar.h"

constexpr uint16_t SCREEN_WIDTH = 800;
constexpr uint16_t SCREEN_HEIGHT = 480;

/*
 * CrowPanel 7-inch V3.0 display hardware configuration.
 *
 * This class and the global lcd object must appear before touch.h
 * because ELECROW's touch.h directly references lcd.width()
 * and lcd.height().
 */
class LGFX : public lgfx::LGFX_Device
{
public:
    lgfx::Bus_RGB bus;
    lgfx::Panel_RGB panel;

    LGFX()
    {
        configureBus();
        configurePanel();

        panel.setBus(&bus);
        setPanel(&panel);
    }

private:
    void configureBus()
    {
        auto config = bus.config();

        config.panel = &panel;

        // Blue data pins
        config.pin_d0 = GPIO_NUM_15;
        config.pin_d1 = GPIO_NUM_7;
        config.pin_d2 = GPIO_NUM_6;
        config.pin_d3 = GPIO_NUM_5;
        config.pin_d4 = GPIO_NUM_4;

        // Green data pins
        config.pin_d5 = GPIO_NUM_9;
        config.pin_d6 = GPIO_NUM_46;
        config.pin_d7 = GPIO_NUM_3;
        config.pin_d8 = GPIO_NUM_8;
        config.pin_d9 = GPIO_NUM_16;
        config.pin_d10 = GPIO_NUM_1;

        // Red data pins
        config.pin_d11 = GPIO_NUM_14;
        config.pin_d12 = GPIO_NUM_21;
        config.pin_d13 = GPIO_NUM_47;
        config.pin_d14 = GPIO_NUM_48;
        config.pin_d15 = GPIO_NUM_45;

        // RGB control pins
        config.pin_henable = GPIO_NUM_41;
        config.pin_vsync = GPIO_NUM_40;
        config.pin_hsync = GPIO_NUM_39;
        config.pin_pclk = GPIO_NUM_0;

        config.freq_write = 15000000;

        // Horizontal timing
        config.hsync_polarity = 0;
        config.hsync_front_porch = 40;
        config.hsync_pulse_width = 48;
        config.hsync_back_porch = 40;

        // Vertical timing
        config.vsync_polarity = 0;
        config.vsync_front_porch = 1;
        config.vsync_pulse_width = 31;
        config.vsync_back_porch = 13;

        config.pclk_active_neg = 1;
        config.de_idle_high = 0;
        config.pclk_idle_high = 0;

        bus.config(config);
    }

    void configurePanel()
    {
        auto config = panel.config();

        config.memory_width = SCREEN_WIDTH;
        config.memory_height = SCREEN_HEIGHT;
        config.panel_width = SCREEN_WIDTH;
        config.panel_height = SCREEN_HEIGHT;
        config.offset_x = 0;
        config.offset_y = 0;

        panel.config(config);
    }
};

/*
 * touch.h requires a globally visible object named lcd.
 * Do not move this declaration below the touch.h include.
 */
LGFX lcd;

#include "touch.h"

namespace
{
    constexpr uint8_t BOARD_ENABLE_PIN = 38;
    constexpr uint8_t BACKLIGHT_PIN = 2;

    constexpr uint8_t BACKLIGHT_CHANNEL = 1;
    constexpr uint16_t BACKLIGHT_FREQUENCY = 300;
    constexpr uint8_t BACKLIGHT_RESOLUTION = 8;
    constexpr uint8_t MIN_BACKLIGHT_PERCENT = 10;

    lv_disp_draw_buf_t drawBuffer;
    lv_disp_drv_t displayDriver;
    lv_indev_drv_t touchDriver;
    bool screenBacklightOff = false;
    bool wakeTouchInProgress = false;
    uint8_t backlightPercent = 100;

    uint8_t backlightDuty(uint8_t percent)
    {
        percent = constrain(percent, MIN_BACKLIGHT_PERCENT, 100);
        return static_cast<uint8_t>((static_cast<uint16_t>(percent) * 255U + 50U) / 100U);
    }

    void setBacklightBrightness(uint8_t percent)
    {
        backlightPercent = constrain(percent, MIN_BACKLIGHT_PERCENT, 100);
        if (!screenBacklightOff)
            ledcWrite(BACKLIGHT_CHANNEL, backlightDuty(backlightPercent));
    }

    void logDisplayMemory(const char* stage)
    {
        Serial.print("[Memory] ");
        Serial.print(stage);
        Serial.print(" internal free=");
        Serial.print(heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
        Serial.print(" largest=");
        Serial.print(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
        Serial.print(" PSRAM free=");
        Serial.println(heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    }

    // Sixteen RGB565 lines are sufficient for LVGL's partial renderer. The
    // former 32-line buffer consumed about 51 KB of scarce internal RAM and
    // left DNS/LwIP with only ~19 KB after UI startup. Keep this DMA source
    // internal, but halve it to return roughly 25 KB to networking.
    constexpr uint32_t DISPLAY_BUFFER_PIXELS =
        SCREEN_WIDTH * SCREEN_HEIGHT / 30;

    lv_color_t displayBuffer[DISPLAY_BUFFER_PIXELS];

    void flushDisplay(
        lv_disp_drv_t* display,
        const lv_area_t* area,
        lv_color_t* colorBuffer)
    {
        const uint32_t width =
            static_cast<uint32_t>(
                area->x2 - area->x1 + 1);

        const uint32_t height =
            static_cast<uint32_t>(
                area->y2 - area->y1 + 1);

        lcd.pushImageDMA(
            area->x1,
            area->y1,
            width,
            height,
            reinterpret_cast<lgfx::rgb565_t*>(
                &colorBuffer->full));

        // LVGL is configured with one partial draw buffer. Do not release that
        // buffer until the RGB565 DMA transfer has consumed it; otherwise LVGL
        // can begin drawing the next region over pixels still being scanned out.
        lcd.waitDMA();
        lv_disp_flush_ready(display);
    }

    void readTouch(
        lv_indev_drv_t* driver,
        lv_indev_data_t* data)
    {
        (void)driver;

        data->state = LV_INDEV_STATE_REL;

        if (!touch_has_signal())
        {
            return;
        }

        const bool touched = touch_touched();

        if (screenBacklightOff && touched)
        {
            ledcWrite(BACKLIGHT_CHANNEL, backlightDuty(backlightPercent));
            screenBacklightOff = false;
            wakeTouchInProgress = true;
            Serial.println("[Display] Screen awakened by touch");
            return;
        }

        // Consume the complete gesture used to wake the panel. LVGL receives a
        // release only after the finger is lifted, preventing the hidden button
        // beneath the wake touch from being activated.
        if (wakeTouchInProgress)
        {
            if (!touched) wakeTouchInProgress = false;
            return;
        }

        if (touched)
        {
            data->state = LV_INDEV_STATE_PR;
            data->point.x = touch_last_x;
            data->point.y = touch_last_y;
        }
    }
}

bool DisplayService::begin(
    ClockService& clockService,
    WeatherService& weatherService,
    AircraftService& aircraftService,
    SatelliteService& satelliteService,
    SolarService& solarService,
    LiveSpotsService& liveSpotsService,
    PotaService& potaService,
    SettingsService& settingsService,
    WiFiService& wifiService)
{
    Serial.println("Initializing CrowPanel display...");
    logDisplayMemory("before display init");

    // Enable the CrowPanel hardware.
    pinMode(BOARD_ENABLE_PIN, OUTPUT);
    digitalWrite(BOARD_ENABLE_PIN, LOW);

    // Initialize the RGB display.
    lcd.begin();
    lcd.fillScreen(TFT_BLACK);
    logDisplayMemory("after RGB driver");

    // Initialize LVGL.
    lv_init();

    // Initialize the GT911 touch controller.
    touch_init();
    logDisplayMemory("after LVGL and touch");

    lv_disp_draw_buf_init(
        &drawBuffer,
        displayBuffer,
        nullptr,
        DISPLAY_BUFFER_PIXELS);

    lv_disp_drv_init(&displayDriver);

    displayDriver.hor_res = SCREEN_WIDTH;
    displayDriver.ver_res = SCREEN_HEIGHT;
    displayDriver.flush_cb = flushDisplay;
    displayDriver.draw_buf = &drawBuffer;

    lv_disp_drv_register(&displayDriver);

    lv_indev_drv_init(&touchDriver);

    touchDriver.type = LV_INDEV_TYPE_POINTER;
    touchDriver.read_cb = readTouch;

    lv_indev_drv_register(&touchDriver);

    // Apply the saved user brightness as soon as PWM is available.
    ledcSetup(
        BACKLIGHT_CHANNEL,
        BACKLIGHT_FREQUENCY,
        BACKLIGHT_RESOLUTION);

    ledcAttachPin(
        BACKLIGHT_PIN,
        BACKLIGHT_CHANNEL);

    screenBacklightOff = false;
    setBacklightBrightness(settingsService.get().displayBrightness);

    SettingsScreen::configureBrightnessCallback(
        [](uint8_t percent) { setBacklightBrightness(percent); });

    HeaderBar::configureScreenOffCallback(
        [this]() { turnOffScreen(); });

    
    Serial.println("Creating Mission Control UI...");

    screenManager.begin(
        clockService,
        weatherService,
        aircraftService,
        satelliteService,
        solarService,
        liveSpotsService,
        potaService,
        settingsService,
        wifiService);

    logDisplayMemory("after all screens");

    Serial.println("Mission Control UI created.");

    lastTickMillis = millis();

    // Force the initial screen to render.
    lv_timer_handler();

    Serial.println("CrowPanel display initialized.");

    return true;
}

void DisplayService::turnOffScreen()
{
    ledcWrite(BACKLIGHT_CHANNEL, 0);
    screenBacklightOff = true;
    wakeTouchInProgress = false;
    Serial.println("[Display] Backlight off; services remain active");
}

void DisplayService::update()
{
    const uint32_t currentMillis = millis();

    const uint32_t elapsedMillis =
        currentMillis - lastTickMillis;

    if (elapsedMillis > 0)
    {
        lv_tick_inc(elapsedMillis);
        lastTickMillis = currentMillis;
    }

    lv_timer_handler();
}
