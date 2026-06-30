#pragma once
#include "Interfaces.h"
#include <Preferences.h>

/**
 * @class PreferencesConfig
 * @brief Implementation of IConfigRepository using ESP32 Preferences (NVS).
 * 
 * Saves servo calibration offsets, invert flags, and API keys to the 
 * ESP32's non-volatile storage so they persist across reboots.
 */
class PreferencesConfig : public IConfigRepository {
public:
    /**
     * @brief Constructs a new Preferences Config repository.
     */
    PreferencesConfig();

    /**
     * @brief Destroys the Preferences Config repository.
     */
    ~PreferencesConfig() override = default;

    /**
     * @brief Initializes the NVS namespace and loads settings into memory.
     */
    void begin() override;
    
    /**
     * @brief Retrieves the configuration for a specific servo.
     * @param index Servo index (0-3).
     * @return Const reference to the cached ServoConfig (no copy).
     */
    const ServoConfig& getServoConfig(int index) override;

    /**
     * @brief Saves the configuration for a specific servo to NVS.
     * @param index Servo index (0-3).
     * @param config The new ServoConfig struct to save.
     */
    void setServoConfig(int index, const ServoConfig& config) override;

    /**
     * @brief Retrieves the saved OpenWeather API key.
     * @return The API key string.
     */
    String getOpenWeatherKey() override;

    /**
     * @brief Saves the OpenWeather API key to NVS.
     * @param key The new API key string.
     */
    void setOpenWeatherKey(const String& key) override;

    /**
     * @brief Retrieves the saved latitude for weather queries.
     * @return Latitude as a float.
     */
    float getLatitude() override;

    /**
     * @brief Saves the latitude to NVS.
     * @param lat Latitude float.
     */
    void setLatitude(float lat) override;

    /**
     * @brief Retrieves the saved longitude for weather queries.
     * @return Longitude as a float.
     */
    float getLongitude() override;

    /**
     * @brief Saves the longitude to NVS.
     * @param lon Longitude float.
     */
    void setLongitude(float lon) override;

private:
    /**
     * @brief Writes default configuration values to NVS on first boot.
     */
    void loadDefaults();
    
    Preferences preferences;
    ServoConfig servoConfigs[4]; ///< In-memory cache of servo configs
};
