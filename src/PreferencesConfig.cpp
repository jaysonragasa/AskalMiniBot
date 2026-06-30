/**
 * @file PreferencesConfig.cpp
 * @brief IConfigRepository implementation backed by ESP32 NVS (Preferences).
 *
 * Loads factory defaults on first boot, then caches servo configs in RAM and
 * persists changes to the "minibot" NVS namespace so calibration and weather
 * settings survive reboots.
 */

#include "PreferencesConfig.h"
#include "config.h"

// -------------------------------------------------------------------------
// PreferencesConfig Constructor
// -------------------------------------------------------------------------
PreferencesConfig::PreferencesConfig() {
}

// -------------------------------------------------------------------------
// PreferencesConfig::begin
// Initializes the NVS memory and loads saved config.
// -------------------------------------------------------------------------
void PreferencesConfig::begin() {
    // Open the "minibot" namespace in Read/Write mode
    preferences.begin("minibot", false);
    
    // Check if we have initialized this ESP32 before
    if (!preferences.getBool("init", false)) {
        // First boot: load hardcoded defaults from config.h and save them to NVS
        loadDefaults();
        preferences.putBool("init", true);
    } else {
        // Subsequent boots: Load saved values into the RAM cache
        for (int i = 0; i < 4; i++) {
            char keyPin[16], keyOffset[16], keyInv[16];
            snprintf(keyPin, sizeof(keyPin), "s%d_pin", i);
            snprintf(keyOffset, sizeof(keyOffset), "s%d_off", i);
            snprintf(keyInv, sizeof(keyInv), "s%d_inv", i);
            
            // Read from NVS, falling back to cached default if missing
            servoConfigs[i].pin = preferences.getInt(keyPin, getServoConfig(i).pin);
            servoConfigs[i].offset = preferences.getFloat(keyOffset, 0.0f);
            servoConfigs[i].invert = preferences.getChar(keyInv, 1);
            
            // Fallback for newer config fields that might not exist in older NVS saves
            servoConfigs[i].maxAngle = preferences.isKey("s_maxA") ? preferences.getFloat("s_maxA", DEFAULT_SERVO_CONFIGS[i].maxAngle) : DEFAULT_SERVO_CONFIGS[i].maxAngle;
            servoConfigs[i].enabled = preferences.isKey("s_en") ? preferences.getChar("s_en", 1) : 1;
        }
    }
}

// -------------------------------------------------------------------------
// PreferencesConfig::loadDefaults
// Populates RAM cache with hardcoded default values.
// -------------------------------------------------------------------------
void PreferencesConfig::loadDefaults() {
    for (int i = 0; i < 4; i++) {
        servoConfigs[i] = DEFAULT_SERVO_CONFIGS[i];
    }
    
    for (int i = 0; i < 4; i++) {
        setServoConfig(i, servoConfigs[i]);
    }
}

// -------------------------------------------------------------------------
// PreferencesConfig::getServoConfig
// Retrieves a specific servo's configuration from RAM.
// -------------------------------------------------------------------------
ServoConfig PreferencesConfig::getServoConfig(int index) {
    if (index >= 0 && index < 4) {
        return servoConfigs[index];
    }
    return DEFAULT_SERVO_CONFIGS[0];
}

// -------------------------------------------------------------------------
// PreferencesConfig::setServoConfig
// Saves a specific servo's configuration to RAM and NVS.
// -------------------------------------------------------------------------
void PreferencesConfig::setServoConfig(int index, const ServoConfig& config) {
    if (index >= 0 && index < 4) {
        servoConfigs[index] = config;
        
        char keyPin[16], keyOffset[16], keyInv[16];
        snprintf(keyPin, sizeof(keyPin), "s%d_pin", index);
        snprintf(keyOffset, sizeof(keyOffset), "s%d_off", index);
        snprintf(keyInv, sizeof(keyInv), "s%d_inv", index);
        
        preferences.putInt(keyPin, config.pin);
        preferences.putFloat(keyOffset, config.offset);
        preferences.putChar(keyInv, config.invert);
        // maxAngle and enabled could also be saved if needed, but keeping it simple for now
    }
}

// -------------------------------------------------------------------------
// PreferencesConfig::getOpenWeatherKey
// Retrieves the saved OpenWeatherMap API key from NVS.
// -------------------------------------------------------------------------
String PreferencesConfig::getOpenWeatherKey() {
    return preferences.getString("ow_key", "");
}

// -------------------------------------------------------------------------
// PreferencesConfig::setOpenWeatherKey
// Saves the OpenWeatherMap API key to NVS.
// -------------------------------------------------------------------------
void PreferencesConfig::setOpenWeatherKey(const String& key) {
    preferences.putString("ow_key", key);
}

float PreferencesConfig::getLatitude() {
    return preferences.getFloat("lat", 0.0f);
}

void PreferencesConfig::setLatitude(float lat) {
    preferences.putFloat("lat", lat);
}

float PreferencesConfig::getLongitude() {
    return preferences.getFloat("lon", 0.0f);
}

void PreferencesConfig::setLongitude(float lon) {
    preferences.putFloat("lon", lon);
}
