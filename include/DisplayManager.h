#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "Interfaces.h" // For JoystickData

enum class EyeEmotion {
    IDLE,
    WANDERING,
    LAUGHING,
    SAD,
    CURIOUS,
    SHOCKED,
    HAPPY,
    ANGRY
};

struct EyeParams {
    float xOffset;
    float yOffset;
    float width;
    float height;
    float radius;
};

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

    static void displayTask(void* parameter);
    void renderEyes();
    void renderWeather();
    void fetchWeather();
    
    // Weather icon drawing helpers
    void drawSun(int cx, int cy, float t);
    void drawCloud(int cx, int cy, float t);
    void drawRain(int cx, int cy, float t);
    
    void updateEyeLogic(float dt);

public:
    DisplayManager(IConfigRepository& configRepo);

    void begin();
    
    // Called from main loop (Core 1) to pass data to the display manager (Core 0)
    void updateData(const String& ipAddress, uint32_t loopTimeMs, const JoystickData& inputs, int gaitIndex);
};
