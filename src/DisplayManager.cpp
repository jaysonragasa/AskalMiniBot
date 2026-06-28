#include "DisplayManager.h"
#include "config.h"
#include <Wire.h>

DisplayManager::DisplayManager() : display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1), isInitialized(false), bootTime(0) {
}

void DisplayManager::begin() {
    Wire.begin(OLED_SDA, OLED_SCL);
    Wire.setTimeOut(20); // Fail fast! Prevents 11-second loop delays if OLED crashes
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed (or not found)"));
        return;
    }
    
    isInitialized = true;
    bootTime = millis();
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(F("Askal"));
    display.println(F("Mini Bot"));
    display.display();
}

void DisplayManager::update(const String& ipAddress, uint32_t loopTimeMs) {
    if (!isInitialized) return;

    display.clearDisplay();
    
    if (millis() - bootTime > 3000) {
        // Draw static rounded rectangle eyes
        // OLED is 128x64. Eye size: 30x40. Corner radius: 8
        // Left Eye
        display.fillRoundRect(24, 12, 30, 40, 8, SSD1306_WHITE);
        // Right Eye
        display.fillRoundRect(74, 12, 30, 40, 8, SSD1306_WHITE);
    } else {
        // Show boot status screen
        display.setTextSize(2);
        display.setCursor(0, 0);
        display.println(F("Askal"));
        display.println(F("Mini Bot"));
        
        display.setTextSize(1);
        display.setCursor(0, 40);
        display.print(F("IP: "));
        display.println(ipAddress);
        
        display.setCursor(0, 50);
        display.print(F("Loop: "));
        display.print(loopTimeMs);
        display.println(F(" ms"));
    }
    
    display.display();
}
