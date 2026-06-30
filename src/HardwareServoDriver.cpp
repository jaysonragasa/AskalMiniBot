/**
 * @file HardwareServoDriver.cpp
 * @brief IServoDriver implementation using native ESP32 LEDC.
 *
 * Bypasses the ESP32Servo library to avoid MCPWM LoadProhibited panics on ESP32-S3.
 * Automatically supports both Arduino Core 2.x and 3.x LEDC APIs.
 */

#include "HardwareServoDriver.h"

HardwareServoDriver::HardwareServoDriver() {
    for (int i = 0; i < 4; i++) {
        servoPins[i] = -1;
    }
}

void HardwareServoDriver::begin() {
    // No global timer allocation needed for native LEDC.
}

void HardwareServoDriver::attach(int index, int pin) {
    if (index >= 0 && index < 4) {
        servoPins[index] = pin;
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
        // Core 3.x API
        ledcAttach(pin, 50, 14); // 50Hz, 14-bit
#else
        // Core 2.x API
        ledcSetup(index, 50, 14); // Use index as channel (0-3)
        ledcAttachPin(pin, index);
#endif
    }
}

void HardwareServoDriver::detach(int index) {
    if (index >= 0 && index < 4 && servoPins[index] >= 0) {
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
        ledcDetach(servoPins[index]);
#else
        ledcDetachPin(servoPins[index]);
#endif
        servoPins[index] = -1;
    }
}

bool HardwareServoDriver::isAttached(int index) {
    if (index >= 0 && index < 4) {
        return servoPins[index] >= 0;
    }
    return false;
}

void HardwareServoDriver::write(int index, int angle) {
    if (index >= 0 && index < 4 && servoPins[index] >= 0) {
        if (angle < 0) angle = 0;
        if (angle > 180) angle = 180;
        
        // 50Hz = 20ms = 20000us
        // 14-bit resolution = max duty 16383
        // 500us = (500/20000) * 16383 = 410
        // 2500us = (2500/20000) * 16383 = 2048
        int duty = map(angle, 0, 180, 410, 2048);
        
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
        ledcWrite(servoPins[index], duty);
#else
        ledcWrite(index, duty);
#endif
    }
}
