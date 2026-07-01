#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "Interfaces.h" // For JoystickData

/**
 * @enum EyeEmotion
 * @brief States for the procedural eye animation engine.
 */
enum class EyeEmotion {
    IDLE,
    WANDERING,
    LAUGHING,
    SAD,
    CURIOUS,
    SHOCKED,
    HAPPY,
    ANGRY,
    SLEEPING
};

/**
 * @struct EyeParams
 * @brief Target physical dimensions for rendering a single eye.
 */
struct EyeParams {
    float xOffset;
    float yOffset;
    float width;
    float height;
    float radius;
};

/**
 * @class DisplayManager
 * @brief Controls the OLED display on a separate FreeRTOS core.
 * 
 * Handles procedural eye animations, boot screens, and fetching/rendering 
 * OpenWeather API data without blocking the main Kinematics loop.
 */
class DisplayManager {
private:
    Adafruit_SSD1306 display;
    bool isInitialized;
    unsigned long bootTime;
    
    IConfigRepository& config;

    // Dual-core FreeRTOS task handle
    TaskHandle_t displayTaskHandle;
    
    // Shared state variables
    JoystickData currentInputs;
    String currentIpAddress;
    uint32_t currentLoopTimeMs;
    int currentGaitIndex;
    
    // Eye Animation State
    EyeEmotion currentEmotion;
    unsigned long lastInputTime;
    unsigned long lastWanderChange;
    
    EyeParams leftEyeTarget;
    EyeParams rightEyeTarget;
    EyeParams leftEyeCurrent;
    EyeParams rightEyeCurrent;
    
    // Weather State
    String weatherTemp;
    String weatherCond;
    String locationName;
    unsigned long lastWeatherFetch;
    float weatherAnimTime;

    /**
     * @brief The FreeRTOS task function running on Core 0.
     * @param parameter Pointer to the DisplayManager instance.
     */
    static void displayTask(void* parameter);

    /**
     * @brief Renders the current state of the eyes based on the animation targets.
     */
    void renderEyes();

    /**
     * @brief Renders the current weather data and animations.
     */
    void renderWeather();

    /**
     * @brief Performs an HTTP GET request to OpenWeatherMap.
     */
    void fetchWeather();
    
    // Weather icon drawing helpers
    /**
     * @brief Draws a sun icon (Clear weather).
     */
    void drawSun(int cx, int cy, float t);

    /**
     * @brief Draws a cloud icon.
     */
    void drawCloud(int cx, int cy, float t);

    /**
     * @brief Draws a rain icon (Cloud + Raindrops).
     */
    void drawRain(int cx, int cy, float t);
    
    /**
     * @brief Updates the procedural logic and emotion states for the eyes.
     * @param dt Delta time since the last frame.
     */
    void updateEyeLogic(float dt);

public:
    /**
     * @brief Constructs a new Display Manager.
     * @param configRepo Reference to the configuration storage.
     */
    DisplayManager(IConfigRepository& configRepo);

    /**
     * @brief Initializes the OLED display and starts the FreeRTOS task.
     */
    void begin();
    
    /**
     * @brief Updates the shared state data used by the display task.
     * Called from the main loop (Core 1) to pass data to the display manager (Core 0).
     * @param ipAddress Current WiFi IP address.
     * @param loopTimeMs Main loop execution time.
     * @param inputs Current joystick inputs.
     * @param gaitIndex Active gait/pose index.
     */
    void updateData(const String& ipAddress, uint32_t loopTimeMs, const JoystickData& inputs, int gaitIndex);
};
