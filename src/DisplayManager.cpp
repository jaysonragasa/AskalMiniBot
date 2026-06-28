#include "DisplayManager.h"
#include "config.h"
#include <Wire.h>
#include <math.h>

// Helper for smooth exponential interpolation
static void smoothApproach(float& current, float target, float speed, float dt) {
    current += (target - current) * speed * dt;
}

DisplayManager::DisplayManager() : display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1), isInitialized(false), bootTime(0), displayTaskHandle(NULL) {
    currentInputs = {0,0,0,0};
    currentIpAddress = "";
    currentLoopTimeMs = 0;
    
    currentEmotion = EyeEmotion::IDLE;
    lastInputTime = 0;
    lastWanderChange = 0;
    
    EyeParams defaultEye = {0, 0, 30, 40, 8};
    leftEyeTarget = defaultEye;
    rightEyeTarget = defaultEye;
    leftEyeCurrent = defaultEye;
    rightEyeCurrent = defaultEye;
}

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
    
    // Create the FreeRTOS task pinned to Core 0
    xTaskCreatePinnedToCore(
        DisplayManager::displayTask,
        "OLED_Task",
        4096,           // Stack size
        this,           // Parameter
        1,              // Priority
        &displayTaskHandle,
        0               // Core 0 (Main runs on Core 1)
    );
}

void DisplayManager::updateData(const String& ipAddress, uint32_t loopTimeMs, const JoystickData& inputs) {
    currentIpAddress = ipAddress;
    currentLoopTimeMs = loopTimeMs;
    currentInputs = inputs;
}

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
            } else {
                // Show Eyes
                self->updateEyeLogic(dt);
                self->renderEyes();
            }
            
            self->display.display();
        }
        
        // 50ms delay = ~20 FPS for OLED updates. This prevents starving Core 0.
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

void DisplayManager::updateEyeLogic(float dt) {
    unsigned long now = millis();
    bool hasInput = (fabs(currentInputs.throttle) > 0.05f || 
                     fabs(currentInputs.yaw) > 0.05f || 
                     fabs(currentInputs.pitch) > 0.05f || 
                     fabs(currentInputs.roll) > 0.05f);
                     
    if (hasInput) {
        lastInputTime = now;
        currentEmotion = EyeEmotion::IDLE; // Reset to IDLE/LOOKING when moving
    } else if (now - lastInputTime > 3000) {
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

void DisplayManager::renderEyes() {
    // Base centers
    int leftCenterX = 39;
    int rightCenterX = 89;
    int centerY = 32;
    
    // Left eye bounds
    int lx = leftCenterX + leftEyeCurrent.xOffset - (leftEyeCurrent.width / 2);
    int ly = centerY + leftEyeCurrent.yOffset - (leftEyeCurrent.height / 2);
    
    // Right eye bounds
    int rx = rightCenterX + rightEyeCurrent.xOffset - (rightEyeCurrent.width / 2);
    int ry = centerY + rightEyeCurrent.yOffset - (rightEyeCurrent.height / 2);
    
    display.fillRoundRect(lx, ly, leftEyeCurrent.width, leftEyeCurrent.height, leftEyeCurrent.radius, SSD1306_WHITE);
    display.fillRoundRect(rx, ry, rightEyeCurrent.width, rightEyeCurrent.height, rightEyeCurrent.radius, SSD1306_WHITE);
}
