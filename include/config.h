#pragma once

// -------------------------------------------------------------------------
// COMPILE-TIME CONFIGURATION
// Central place for hardware pins, credentials, and tuning constants that are
// fixed at build time. Runtime-adjustable values (servo trim, weather settings)
// live in the IConfigRepository / NVS instead.
// -------------------------------------------------------------------------

// Wi-Fi Credentials
// NOTE: These are baked into the firmware. Treat this file as a secret and
// avoid committing real production credentials.
#define WIFI_SSID "corp-wifi"
#define WIFI_PASS "1nf1ni8m@InF0r"

// OLED Pins (I2C) - ESP32-S3 Supermini defaults
#define OLED_SDA 6           ///< I2C data pin for the SSD1306 OLED.
#define OLED_SCL 7           ///< I2C clock pin for the SSD1306 OLED.
#define OLED_ADDRESS 0x3C    ///< 7-bit I2C address of the OLED.
#define OLED_WIDTH 128       ///< OLED width in pixels.
#define OLED_HEIGHT 64       ///< OLED height in pixels.

// Uncomment the line below to enable the OLED display
// When undefined, all DisplayManager code is compiled out (see main.cpp).
#define ENABLE_OLED_DISPLAY

#include "Interfaces.h"

// Gait Engine Constants
// Target period of the main loop in milliseconds (20ms => ~50Hz servo update rate).
#define LOOP_DELAY_MS 20

// -------------------------------------------------------------------------
// FACTORY DEFAULT SERVO CONFIGURATIONS
// Written to NVS on first boot (see PreferencesConfig::loadDefaults).
// Fields, in order: pin, maxAngle, invert (1 or -1), enabled (1 or 0), offset.
// Index order is fixed across the whole codebase:
//   0 = Front-Left, 1 = Front-Right, 2 = Hind-Left, 3 = Hind-Right.
// Right-side servos default to inverted (-1) because they are mirrored.
// -------------------------------------------------------------------------
constexpr ServoConfig DEFAULT_SERVO_CONFIGS[4] = {
    { 1,  30.0f,  1, 1, 0.0f }, // Front-Left
    { 2,  30.0f, -1, 1, 0.0f }, // Front-Right
    { 13, 30.0f,  1, 1, 0.0f }, // Hind-Left
    { 14, 30.0f, -1, 1, 0.0f }  // Hind-Right
};
