#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

class DisplayManager {
private:
    Adafruit_SSD1306 display;
    bool isInitialized;
    unsigned long bootTime;

public:
    DisplayManager();

    void begin();
    void update(const String& ipAddress, uint32_t loopTimeMs);
};
