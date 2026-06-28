#pragma once

// Wi-Fi Credentials
#define WIFI_SSID "corp-wifi"
#define WIFI_PASS "1nf1ni8m@InF0r"

// OLED Pins (I2C) - ESP32-S3 Supermini defaults
#define OLED_SDA 6
#define OLED_SCL 7
#define OLED_ADDRESS 0x3C
#define OLED_WIDTH 128
#define OLED_HEIGHT 64

// Uncomment the line below to enable the OLED display
#define ENABLE_OLED_DISPLAY

#include "Interfaces.h"

// Gait Engine Constants
#define LOOP_DELAY_MS 20

// Factory Default Configurations for Servos
// pin, maxAngle, invert (1 or -1), enabled (1 or 0), offset
constexpr ServoConfig DEFAULT_SERVO_CONFIGS[4] = {
    { 1,  30.0f,  1, 1, 0.0f }, // Front-Left
    { 2,  30.0f, -1, 1, 0.0f }, // Front-Right
    { 13, 30.0f,  1, 1, 0.0f }, // Hind-Left
    { 14, 30.0f, -1, 1, 0.0f }  // Hind-Right
};

