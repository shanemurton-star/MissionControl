/*******************************************************************************
 * Touch libraries:
 * FT6X36: https://github.com/strange-v/FT6X36.git
 * GT911: https://github.com/TAMCTec/gt911-arduino.git
 * XPT2046: https://github.com/PaulStoffregen/XPT2046_Touchscreen.git
 ******************************************************************************/

#pragma once

/* Uncomment for FT6X36 */
// #define TOUCH_FT6X36
// #define TOUCH_FT6X36_SCL 19
// #define TOUCH_FT6X36_SDA 18
// #define TOUCH_FT6X36_INT 39
// #define TOUCH_SWAP_XY
// #define TOUCH_MAP_X1 480
// #define TOUCH_MAP_X2 0
// #define TOUCH_MAP_Y1 0
// #define TOUCH_MAP_Y2 320

/* GT911 configuration for the CrowPanel 7-inch display */
#define TOUCH_GT911
#define TOUCH_GT911_SCL 20
#define TOUCH_GT911_SDA 19
#define TOUCH_GT911_INT -1
#define TOUCH_GT911_RST -1
#define TOUCH_GT911_ROTATION ROTATION_NORMAL
#define TOUCH_MAP_X1 800
#define TOUCH_MAP_X2 0
#define TOUCH_MAP_Y1 480
#define TOUCH_MAP_Y2 0

/* Uncomment for XPT2046 */
// #define TOUCH_XPT2046
// #define TOUCH_XPT2046_SCK 12
// #define TOUCH_XPT2046_MISO 13
// #define TOUCH_XPT2046_MOSI 11
// #define TOUCH_XPT2046_CS 38
// #define TOUCH_XPT2046_INT 18
// #define TOUCH_XPT2046_ROTATION 0
// #define TOUCH_MAP_X1 4000
// #define TOUCH_MAP_X2 100
// #define TOUCH_MAP_Y1 100
// #define TOUCH_MAP_Y2 4000

int touch_last_x = 0;
int touch_last_y = 0;

#if defined(TOUCH_FT6X36)

#include <Wire.h>
#include <FT6X36.h>

FT6X36 ts(&Wire, TOUCH_FT6X36_INT);

bool touch_touched_flag = true;
bool touch_released_flag = true;

#elif defined(TOUCH_GT911)

#include <Wire.h>
#include <TAMC_GT911.h>

TAMC_GT911 ts(
    TOUCH_GT911_SDA,
    TOUCH_GT911_SCL,
    TOUCH_GT911_INT,
    TOUCH_GT911_RST,
    max(TOUCH_MAP_X1, TOUCH_MAP_X2),
    max(TOUCH_MAP_Y1, TOUCH_MAP_Y2));

uint8_t touch_gt911_address = GT911_ADDR1;
bool touch_gt911_online = false;

bool touch_gt911_probe(uint8_t address)
{
    Wire.beginTransmission(address);
    return Wire.endTransmission() == 0;
}

#elif defined(TOUCH_XPT2046)

#include <SPI.h>
#include <XPT2046_Touchscreen.h>

XPT2046_Touchscreen ts(
    TOUCH_XPT2046_CS,
    TOUCH_XPT2046_INT);

#endif

#if defined(TOUCH_FT6X36)

void touch(TPoint point, TEvent event)
{
    if (event != TEvent::Tap &&
        event != TEvent::DragStart &&
        event != TEvent::DragMove &&
        event != TEvent::DragEnd)
    {
        return;
    }

#if defined(TOUCH_SWAP_XY)
    touch_last_x = map(
        point.y,
        TOUCH_MAP_X1,
        TOUCH_MAP_X2,
        0,
        lcd.width());

    touch_last_y = map(
        point.x,
        TOUCH_MAP_Y1,
        TOUCH_MAP_Y2,
        0,
        lcd.height());
#else
    touch_last_x = map(
        point.x,
        TOUCH_MAP_X1,
        TOUCH_MAP_X2,
        0,
        lcd.width());

    touch_last_y = map(
        point.y,
        TOUCH_MAP_Y1,
        TOUCH_MAP_Y2,
        0,
        lcd.height());
#endif

    switch (event)
    {
        case TEvent::Tap:
            Serial.println("Tap");
            touch_touched_flag = true;
            touch_released_flag = true;
            break;

        case TEvent::DragStart:
            Serial.println("DragStart");
            touch_touched_flag = true;
            break;

        case TEvent::DragMove:
            Serial.println("DragMove");
            touch_touched_flag = true;
            break;

        case TEvent::DragEnd:
            Serial.println("DragEnd");
            touch_released_flag = true;
            break;

        default:
            Serial.println("UNKNOWN");
            break;
    }
}

#endif

void touch_init()
{
#if defined(TOUCH_FT6X36)

    Wire.begin(
        TOUCH_FT6X36_SDA,
        TOUCH_FT6X36_SCL);

    ts.begin();
    ts.registerTouchHandler(touch);

#elif defined(TOUCH_GT911)

    // A firmware upload resets the ESP32 without necessarily power-cycling the
    // external GT911. Restart the I2C controller cleanly so a stale bus state
    // cannot leave an otherwise running UI without touch input.
    Wire.end();
    delay(5);
    ts.begin(GT911_ADDR1);
    Wire.setTimeOut(20);
    touch_gt911_address = GT911_ADDR1;
    touch_gt911_online = touch_gt911_probe(touch_gt911_address);
    if (!touch_gt911_online)
    {
        ts.begin(GT911_ADDR2);
        Wire.setTimeOut(20);
        touch_gt911_address = GT911_ADDR2;
        touch_gt911_online = touch_gt911_probe(touch_gt911_address);
    }
    ts.setRotation(TOUCH_GT911_ROTATION);
    Serial.print("[Touch] GT911 ");
    if (touch_gt911_online)
    {
        Serial.print("online at 0x");
        Serial.println(touch_gt911_address, HEX);
    }
    else
    {
        Serial.println("not responding on I2C");
    }

#elif defined(TOUCH_XPT2046)

    SPI.begin(
        TOUCH_XPT2046_SCK,
        TOUCH_XPT2046_MISO,
        TOUCH_XPT2046_MOSI,
        TOUCH_XPT2046_CS);

    ts.begin();
    ts.setRotation(TOUCH_XPT2046_ROTATION);

#endif
}

bool touch_has_signal()
{
#if defined(TOUCH_FT6X36)

    ts.loop();
    return touch_touched_flag || touch_released_flag;

#elif defined(TOUCH_GT911)

    return touch_gt911_online;

#elif defined(TOUCH_XPT2046)

    return ts.tirqTouched();

#else

    return false;

#endif
}

bool touch_touched()
{
#if defined(TOUCH_FT6X36)

    if (touch_touched_flag)
    {
        touch_touched_flag = false;
        return true;
    }

    return false;

#elif defined(TOUCH_GT911)

    ts.read();

    if (!ts.isTouched)
    {
        return false;
    }

#if defined(TOUCH_SWAP_XY)
    touch_last_x = map(
        ts.points[0].y,
        TOUCH_MAP_X1,
        TOUCH_MAP_X2,
        0,
        lcd.width() - 1);

    touch_last_y = map(
        ts.points[0].x,
        TOUCH_MAP_Y1,
        TOUCH_MAP_Y2,
        0,
        lcd.height() - 1);
#else
    touch_last_x = map(
        ts.points[0].x,
        TOUCH_MAP_X1,
        TOUCH_MAP_X2,
        0,
        lcd.width() - 1);

    touch_last_y = map(
        ts.points[0].y,
        TOUCH_MAP_Y1,
        TOUCH_MAP_Y2,
        0,
        lcd.height() - 1);
#endif

    return true;

#elif defined(TOUCH_XPT2046)

    if (!ts.touched())
    {
        return false;
    }

    TS_Point point = ts.getPoint();

#if defined(TOUCH_SWAP_XY)
    touch_last_x = map(
        point.y,
        TOUCH_MAP_X1,
        TOUCH_MAP_X2,
        0,
        lcd.width() - 1);

    touch_last_y = map(
        point.x,
        TOUCH_MAP_Y1,
        TOUCH_MAP_Y2,
        0,
        lcd.height() - 1);
#else
    touch_last_x = map(
        point.x,
        TOUCH_MAP_X1,
        TOUCH_MAP_X2,
        0,
        lcd.width() - 1);

    touch_last_y = map(
        point.y,
        TOUCH_MAP_Y1,
        TOUCH_MAP_Y2,
        0,
        lcd.height() - 1);
#endif

    return true;

#else

    return false;

#endif
}

bool touch_released()
{
#if defined(TOUCH_FT6X36)

    if (touch_released_flag)
    {
        touch_released_flag = false;
        return true;
    }

    return false;

#elif defined(TOUCH_GT911)

    return true;

#elif defined(TOUCH_XPT2046)

    return true;

#else

    return false;

#endif
}
