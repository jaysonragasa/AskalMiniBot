#include "PreferencesConfig.h"
#include "config.h"

PreferencesConfig::PreferencesConfig() {
}

void PreferencesConfig::begin() {
    preferences.begin("minibot", false);
    
    // Check if initialized
    if (!preferences.getBool("init", false)) {
        loadDefaults();
        preferences.putBool("init", true);
    } else {
        for (int i = 0; i < 4; i++) {
            char keyPin[16], keyOffset[16], keyInv[16];
            snprintf(keyPin, sizeof(keyPin), "s%d_pin", i);
            snprintf(keyOffset, sizeof(keyOffset), "s%d_off", i);
            snprintf(keyInv, sizeof(keyInv), "s%d_inv", i);
            
            servoConfigs[i].pin = preferences.getInt(keyPin, getServoConfig(i).pin);
            servoConfigs[i].offset = preferences.getFloat(keyOffset, 0.0f);
            servoConfigs[i].invert = preferences.getChar(keyInv, 1);
            
            // Check existence for keys that might not have been saved yet
            servoConfigs[i].maxAngle = preferences.isKey("s_maxA") ? preferences.getFloat("s_maxA", DEFAULT_SERVO_CONFIGS[i].maxAngle) : DEFAULT_SERVO_CONFIGS[i].maxAngle;
            servoConfigs[i].enabled = preferences.isKey("s_en") ? preferences.getChar("s_en", 1) : 1;
        }
    }
}

void PreferencesConfig::loadDefaults() {
    for (int i = 0; i < 4; i++) {
        servoConfigs[i] = DEFAULT_SERVO_CONFIGS[i];
    }
    
    for (int i = 0; i < 4; i++) {
        setServoConfig(i, servoConfigs[i]);
    }
}

ServoConfig PreferencesConfig::getServoConfig(int index) {
    if (index >= 0 && index < 4) {
        return servoConfigs[index];
    }
    return DEFAULT_SERVO_CONFIGS[0];
}

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
