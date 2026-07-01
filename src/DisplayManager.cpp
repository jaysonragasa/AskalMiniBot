/**
 * @file DisplayManager.cpp
 * @brief OLED rendering implementation running on a dedicated FreeRTOS task (Core 0).
 *
 * Renders procedural "eyes" with an emotion state machine, a boot splash, and an
 * OpenWeather screen. The render task is pinned to Core 0 so its blocking I2C
 * writes and HTTP weather fetches never introduce jitter into the servo PWM /
 * kinematics loop running on Core 1. Shared state is published via updateData().
 */

#include "DisplayManager.h"
#include "config.h"
#include <Wire.h>
#include <math.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

/**
 * @brief Smooth exponential interpolation toward a target (ease-out).
 *
 * Moves @p current a fraction (speed * dt) of the remaining distance to
 * @p target each call, producing a natural ease-out animation curve.
 * @param current Reference to the value being animated; updated in place.
 * @param target The value to approach.
 * @param speed Interpolation rate (higher converges faster).
 * @param dt Delta time in seconds since the last update.
 */
static void smoothApproach(float& current, float target, float speed, float dt) {
    current += (target - current) * speed * dt;
}

// -------------------------------------------------------------------------
// DisplayManager Constructor
// -------------------------------------------------------------------------
DisplayManager::DisplayManager(IConfigRepository& configRepo) : display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1), isInitialized(false), bootTime(0), config(configRepo), displayTaskHandle(NULL) {
    currentInputs = {0,0,0,0};
    currentIpAddress = "";
    currentLoopTimeMs = 0;
    currentGaitIndex = -1;
    
    currentEmotion = EyeEmotion::IDLE;
    lastInputTime = 0;
    lastWanderChange = 0;
    
    EyeParams defaultEye = {0, 0, 30, 40, 8};
    leftEyeTarget = defaultEye;
    rightEyeTarget = defaultEye;
    leftEyeCurrent = defaultEye;
    rightEyeCurrent = defaultEye;
}

// -------------------------------------------------------------------------
// DisplayManager::begin
// Initializes the OLED and starts the FreeRTOS display task.
// -------------------------------------------------------------------------
void DisplayManager::begin() {
    Wire.begin(OLED_SDA, OLED_SCL);
    Wire.setClock(400000); // 400kHz for fast OLED updates
    Wire.setTimeOut(20); 
    
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        return;
    }
    
    isInitialized = true;
    bootTime = millis();
    lastInputTime = millis();
    lastWanderChange = millis();
    
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(F("Askal"));
    display.println(F("Mini Bot"));
    display.display();
    
    // -------------------------------------------------------------------------
    // Create the FreeRTOS task pinned to Core 0.
    // The main loop (WiFi, WebServer, Kinematics) runs on Core 1 by default.
    // Offloading the display ensures that I2C delays don't cause jitter in servo PWM.
    // -------------------------------------------------------------------------
    xTaskCreatePinnedToCore(
        DisplayManager::displayTask,
        "OLED_Task",
        4096,           // Stack size in words
        this,           // Pass this instance as parameter
        1,              // Priority (Low)
        &displayTaskHandle,
        0               // Pin to Core 0
    );
}

// -------------------------------------------------------------------------
// DisplayManager::updateData
// Updates shared state variables from the main core.
// -------------------------------------------------------------------------
void DisplayManager::updateData(const String& ipAddress, uint32_t loopTimeMs, const JoystickData& inputs, int gaitIndex) {
    currentIpAddress = ipAddress;
    currentLoopTimeMs = loopTimeMs;
    currentInputs = inputs;
    currentGaitIndex = gaitIndex;
}

// -------------------------------------------------------------------------
// DisplayManager::displayTask
// The FreeRTOS task function running on Core 0.
// -------------------------------------------------------------------------
void DisplayManager::displayTask(void* parameter) {
    DisplayManager* self = static_cast<DisplayManager*>(parameter);
    
    unsigned long lastTick = millis();
    
    while (true) {
        if (self->isInitialized) {
            unsigned long now = millis();
            float dt = (now - lastTick) / 1000.0f;
            lastTick = now;
            
            self->display.clearDisplay();
            
            if (now - self->bootTime < 3000) {
                // Show boot screen
                self->display.setTextSize(2);
                self->display.setCursor(0, 0);
                self->display.println(F("Askal"));
                self->display.println(F("Mini Bot"));
                
                self->display.setTextSize(1);
                self->display.setCursor(0, 40);
                self->display.print(F("IP: "));
                self->display.println(self->currentIpAddress);
                
                self->display.setCursor(0, 50);
                self->display.print(F("Loop: "));
                self->display.print(self->currentLoopTimeMs);
                self->display.println(F(" ms"));
            } else if (self->currentGaitIndex == 10) { // INFO pose (index 10 in allGaits[])
                if (self->lastWeatherFetch == 0 || now - self->lastWeatherFetch > 600000) {
                    // Fetch every 10 mins or on first entry
                    self->display.clearDisplay();
                    self->display.setCursor(0, 20);
                    self->display.setTextSize(1);
                    self->display.println("Fetching");
                    self->display.println("Weather...");
                    self->display.display();
                    
                    self->fetchWeather();
                    self->lastWeatherFetch = now;
                }
                self->display.clearDisplay();
                self->weatherAnimTime += dt;
                self->renderWeather();
            } else {
                // Show Eyes
                self->lastWeatherFetch = 0; // Reset fetch timer so it loads immediately next time
                self->updateEyeLogic(dt);
                self->renderEyes();
            }
            
            self->display.display();
        }
        
        // 50ms delay = ~20 FPS for OLED updates. This prevents starving Core 0.
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

// -------------------------------------------------------------------------
// DisplayManager::updateEyeLogic
// Updates procedural animation targets for the eyes based on inputs.
// -------------------------------------------------------------------------
void DisplayManager::updateEyeLogic(float dt) {
    unsigned long now = millis();
    
    // Check if any joystick input is active (deadzone of 5%)
    bool hasInput = (fabs(currentInputs.throttle) > 0.05f || 
                     fabs(currentInputs.yaw) > 0.05f || 
                     fabs(currentInputs.pitch) > 0.05f || 
                     fabs(currentInputs.roll) > 0.05f);
                     
    // -------------------------------------------------------------------------
    // EMOTION STATE MACHINE
    // -------------------------------------------------------------------------
    if (currentGaitIndex == 6) { // Fold (index 6)
        currentEmotion = EyeEmotion::SLEEPING;
    } else if (currentGaitIndex == 8) { // Pee (index 8)
        currentEmotion = EyeEmotion::HAPPY;
    } else if (currentGaitIndex == 9) { // Scrape (index 9)
        currentEmotion = EyeEmotion::ANGRY;
    } else if (hasInput) {
        // If user is controlling the robot, eyes should lock forward (IDLE)
        lastInputTime = now;
        currentEmotion = EyeEmotion::IDLE; 
    } else if (now - lastInputTime > 3000) {
        // If idle for 3 seconds, start wandering eyes procedurally
        currentEmotion = EyeEmotion::WANDERING;
    }
    
    EyeParams defaultLeft = {0, 0, 30, 40, 8};
    EyeParams defaultRight = {0, 0, 30, 40, 8};
    
    if (currentEmotion == EyeEmotion::WANDERING) {
        if (now - lastWanderChange > 1500) { // Change wander target every 1.5s
            lastWanderChange = now;
            
            // 20% chance to blink or do a preset emotion during wandering
            int r = random(100);
            if (r < 10) {
                // Blink
                leftEyeTarget.height = 4; rightEyeTarget.height = 4;
            } else if (r < 15) {
                // Curious
                leftEyeTarget = defaultLeft;
                rightEyeTarget = {0, -10, 30, 20, 8};
            } else if (r < 20) {
                // Happy/Laughing (squint)
                leftEyeTarget = {0, 0, 30, 10, 5};
                rightEyeTarget = {0, 0, 30, 10, 5};
            } else {
                // Random wander
                int targetX = random(-15, 16);
                int targetY = random(-15, 16);
                leftEyeTarget = defaultLeft;
                leftEyeTarget.xOffset = targetX;
                leftEyeTarget.yOffset = targetY;
                rightEyeTarget = defaultRight;
                rightEyeTarget.xOffset = targetX;
                rightEyeTarget.yOffset = targetY;
            }
        } else if (now - lastWanderChange > 200 && leftEyeTarget.height == 4) {
            // Restore from blink quickly
            leftEyeTarget.height = 40;
            rightEyeTarget.height = 40;
        }
    } else if (currentEmotion == EyeEmotion::SLEEPING) {
        // Sleep state: eyes closed (flat) and lowered
        leftEyeTarget = {0, 10, 30, 4, 2};
        rightEyeTarget = {0, 10, 30, 4, 2};
    } else {
        // IDLE / LOOKING state
        leftEyeTarget = defaultLeft;
        rightEyeTarget = defaultRight;
        
        // Map inputs to eye movement
        // Yaw affects X, Pitch affects Y
        // Inputs range from -100 to +100
        float maxOffset = 15.0f;
        float xOff = (currentInputs.yaw / 100.0f) * maxOffset; 
        float yOff = -(currentInputs.pitch / 100.0f) * maxOffset; // inverted so +pitch = up
        
        // Roll: Shifting X slightly, and skewing Y to simulate rotation
        float rollMag = currentInputs.roll / 100.0f;
        xOff += rollMag * 10.0f; 
        
        leftEyeTarget.xOffset = xOff;
        leftEyeTarget.yOffset = yOff - (rollMag * 8.0f); // Skew left eye
        rightEyeTarget.xOffset = xOff;
        rightEyeTarget.yOffset = yOff + (rollMag * 8.0f); // Skew right eye opposite
        
        // Eyes get bigger when throttling forward (excitement/speed)
        if (currentInputs.throttle > 5.0f) {
            float throttleMag = currentInputs.throttle / 100.0f;
            float extraSize = throttleMag * 15.0f; 
            leftEyeTarget.width += extraSize;
            leftEyeTarget.height += extraSize;
            rightEyeTarget.width += extraSize;
            rightEyeTarget.height += extraSize;
        }
        
        // Blink occasionally even when IDLE (not wandering)
        if (random(1000) < 15) { // Roughly 1.5% chance every 50ms (~every 3.3s on average)
            leftEyeCurrent.height = 4; // Instant blink, it will interpolate back up
            rightEyeCurrent.height = 4;
        }
    }
    
    // Smooth interpolation for both eyes
    float speed = 12.0f; // interpolation speed
    smoothApproach(leftEyeCurrent.xOffset, leftEyeTarget.xOffset, speed, dt);
    smoothApproach(leftEyeCurrent.yOffset, leftEyeTarget.yOffset, speed, dt);
    smoothApproach(leftEyeCurrent.width, leftEyeTarget.width, speed, dt);
    smoothApproach(leftEyeCurrent.height, leftEyeTarget.height, speed, dt);
    
    smoothApproach(rightEyeCurrent.xOffset, rightEyeTarget.xOffset, speed, dt);
    smoothApproach(rightEyeCurrent.yOffset, rightEyeTarget.yOffset, speed, dt);
    smoothApproach(rightEyeCurrent.width, rightEyeTarget.width, speed, dt);
    smoothApproach(rightEyeCurrent.height, rightEyeTarget.height, speed, dt);
}

// -------------------------------------------------------------------------
// DisplayManager::renderEyes
// Draws the current eye state to the OLED display buffer.
// -------------------------------------------------------------------------
void DisplayManager::renderEyes() {
    // -------------------------------------------------------------------------
    // BASE EYE RENDERING
    // -------------------------------------------------------------------------
    int leftCenterX = 39;
    int rightCenterX = 89;
    int centerY = 32; // Vertical center of the 128x64 display
    
    // Calculate top-left bounds for drawing rounded rectangles
    int lx = leftCenterX + leftEyeCurrent.xOffset - (leftEyeCurrent.width / 2);
    int ly = centerY + leftEyeCurrent.yOffset - (leftEyeCurrent.height / 2);
    
    int rx = rightCenterX + rightEyeCurrent.xOffset - (rightEyeCurrent.width / 2);
    int ry = centerY + rightEyeCurrent.yOffset - (rightEyeCurrent.height / 2);
    
    // Draw the white bases
    display.fillRoundRect(lx, ly, leftEyeCurrent.width, leftEyeCurrent.height, leftEyeCurrent.radius, SSD1306_WHITE);
    display.fillRoundRect(rx, ry, rightEyeCurrent.width, rightEyeCurrent.height, rightEyeCurrent.radius, SSD1306_WHITE);
    
    // -------------------------------------------------------------------------
    // EMOTION MASKING
    // We achieve expressions by drawing black geometry OVER the white eyes.
    // -------------------------------------------------------------------------
    if (currentEmotion == EyeEmotion::ANGRY) {
        // Draw black triangles angled downward to mask the top inner corners
        display.fillTriangle(lx + leftEyeCurrent.width/2, ly - 5, lx + leftEyeCurrent.width + 5, ly - 5, lx + leftEyeCurrent.width + 5, ly + leftEyeCurrent.height/2, SSD1306_BLACK);
        display.fillTriangle(rx - 5, ly - 5, rx + rightEyeCurrent.width/2, ly - 5, rx - 5, ly + rightEyeCurrent.height/2, SSD1306_BLACK);
    } else if (currentEmotion == EyeEmotion::HAPPY) {
        // Draw black rectangles over the bottom half to create a smiling squint (^_^)
        display.fillRect(lx, ly + leftEyeCurrent.height/2 + 2, leftEyeCurrent.width, leftEyeCurrent.height/2, SSD1306_BLACK);
        display.fillRect(rx, ry + rightEyeCurrent.height/2 + 2, rightEyeCurrent.width, rightEyeCurrent.height/2, SSD1306_BLACK);
    } else if (currentEmotion == EyeEmotion::SLEEPING) {
        // Draw floating zZZZ
        unsigned long now = millis();
        display.setTextSize(1);
        
        // Animated float based on time
        int bob = sin(now * 0.003f) * 4;
        
        display.setCursor(95, 15 + bob);
        display.print("z");
        display.setCursor(105, 10 + bob);
        display.print("Z");
        display.setCursor(115, 3 + bob);
        display.print("Z");
    }
}

// -------------------------------------------------------------------------
// DisplayManager::fetchWeather
// Performs HTTP GET to OpenWeatherMap and parses JSON.
// -------------------------------------------------------------------------
void DisplayManager::fetchWeather() {
    String apiKey = config.getOpenWeatherKey();
    if (apiKey.length() == 0) {
        weatherTemp = "No API Key";
        weatherCond = "Go to Settings";
        locationName = "";
        return;
    }
    
    float lat = config.getLatitude();
    float lon = config.getLongitude();
    
    String url = "http://api.openweathermap.org/data/2.5/weather?lat=" + String(lat, 4) + "&lon=" + String(lon, 4) + "&appid=" + apiKey + "&units=metric";
    
    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (!error) {
            float temp = doc["main"]["temp"];
            const char* cond = doc["weather"][0]["main"];
            const char* loc = doc["name"];
            weatherTemp = String(temp, 1) + " C";
            weatherCond = String(cond);
            locationName = String(loc);
        } else {
            weatherTemp = "JSON Error";
            weatherCond = "";
            locationName = "";
        }
    } else {
        weatherTemp = "HTTP " + String(httpCode);
        weatherCond = "Failed";
        locationName = "";
    }
    http.end();
}

// -------------------------------------------------------------------------
// DisplayManager::drawSun
// Draws an animated sun icon.
// -------------------------------------------------------------------------
void DisplayManager::drawSun(int cx, int cy, float t) {
    // Draw center circle
    display.fillCircle(cx, cy, 8, SSD1306_WHITE);
    // Draw rotating rays
    for (int i = 0; i < 8; i++) {
        float angle = (i * PI / 4.0f) + (t * 0.5f); // Rotate slowly
        int x1 = cx + cos(angle) * 11;
        int y1 = cy + sin(angle) * 11;
        int x2 = cx + cos(angle) * 16;
        int y2 = cy + sin(angle) * 16;
        display.drawLine(x1, y1, x2, y2, SSD1306_WHITE);
    }
}

// -------------------------------------------------------------------------
// DisplayManager::drawCloud
// Draws an animated cloud icon.
// -------------------------------------------------------------------------
void DisplayManager::drawCloud(int cx, int cy, float t) {
    // Bobbing motion
    int bob = sin(t * 2.0f) * 2;
    cy += bob;
    
    // Draw 3 overlapping filled circles to make a cloud
    display.fillCircle(cx - 8, cy + 3, 6, SSD1306_WHITE);
    display.fillCircle(cx, cy, 8, SSD1306_WHITE);
    display.fillCircle(cx + 8, cy + 3, 6, SSD1306_WHITE);
    // Fill the bottom gap
    display.fillRect(cx - 8, cy + 3, 16, 6, SSD1306_WHITE);
}

// -------------------------------------------------------------------------
// DisplayManager::drawRain
// Draws an animated rain icon.
// -------------------------------------------------------------------------
void DisplayManager::drawRain(int cx, int cy, float t) {
    drawCloud(cx, cy, t);
    
    // Raindrops falling animation
    int bob = sin(t * 2.0f) * 2;
    int base_y = cy + bob + 10;
    
    for (int i = -1; i <= 1; i++) {
        int drop_x = cx + (i * 6);
        // Offset each drop's phase
        float drop_t = t * 15.0f + (i * 2.0f);
        int drop_y = base_y + ((int)drop_t % 10);
        display.drawLine(drop_x, drop_y, drop_x - 2, drop_y + 3, SSD1306_WHITE);
    }
}

// -------------------------------------------------------------------------
// DisplayManager::renderWeather
// Draws the weather info (temp, conditions) and appropriate icon.
// -------------------------------------------------------------------------
void DisplayManager::renderWeather() {
    // Location Name at the top left
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("Loc: ");
    display.println(locationName.length() > 0 ? locationName : "Unknown");
    
    // Temperature in the middle left
    display.setTextSize(2);
    display.setCursor(0, 20);
    display.println(weatherTemp);
    
    // Condition text at bottom left
    display.setTextSize(1);
    display.setCursor(0, 45);
    display.println(weatherCond);
    
    // Animated Icon on the right side
    int iconX = 95;
    int iconY = 32;
    
    String lowerCond = weatherCond;
    lowerCond.toLowerCase();
    
    if (lowerCond.indexOf("rain") >= 0 || lowerCond.indexOf("drizzle") >= 0 || lowerCond.indexOf("thunderstorm") >= 0) {
        drawRain(iconX, iconY, weatherAnimTime);
    } else if (lowerCond.indexOf("cloud") >= 0) {
        drawCloud(iconX, iconY, weatherAnimTime);
    } else if (lowerCond.indexOf("clear") >= 0) {
        drawSun(iconX, iconY, weatherAnimTime);
    } else {
        // Default to cloud if unknown
        drawCloud(iconX, iconY, weatherAnimTime);
    }
}
