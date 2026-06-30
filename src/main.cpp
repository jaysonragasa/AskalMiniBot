#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "config.h"

// SOLID Refactored Modules
#include "PreferencesConfig.h"
#include "HardwareServoDriver.h"
#include "TrotGait.h"
#include "WalkGait.h"
#include "GallopGait.h"
#include "AutoGait.h"
#include "StaticPoses.h"
#include "RobotKinematics.h"
#include "WebUIManager.h"
#ifdef ENABLE_OLED_DISPLAY
#include "DisplayManager.h"
#endif

// Instantiate dependencies
PreferencesConfig configRepo;
HardwareServoDriver servoDriver;
TrotGait trotGait;
WalkGait walkGait;
GallopGait gallopGait;
AutoGait autoGait;
SitPose sitPose;
StretchPose stretchPose;
WavePose wavePose;
PeePose peePose;
ScrapePose scrapePose;
InfoPose infoPose;

IGaitStrategy* allGaits[] = { &trotGait, &walkGait, &gallopGait, &autoGait, &sitPose, &stretchPose, &wavePose, &peePose, &scrapePose, &infoPose };
int numGaits = sizeof(allGaits) / sizeof(allGaits[0]);

// High-level components injected with dependencies
RobotKinematics robot(servoDriver, configRepo, allGaits, numGaits);
WebUIManager webUI(robot, configRepo);
#ifdef ENABLE_OLED_DISPLAY
DisplayManager displayMgr(configRepo);
#endif

unsigned long lastOledUpdate = 0;
unsigned long lastLoopTick = 0;
unsigned long lastSerialLog = 0;
uint32_t currentLoopTime = 0;

void setup() {
    Serial.begin(115200);
    // Add small delay to allow serial to connect via USB CDC if needed
    delay(2000); 
    Serial.println("\nAskalMiniBot Starting...");

    // -------------------------------------------------------------------------
    // 1. HARDWARE INITIALIZATION
    // -------------------------------------------------------------------------
    // Initialize display early to show boot status
#ifdef ENABLE_OLED_DISPLAY
    displayMgr.begin();
#endif

    // Load saved servo calibration and API keys from non-volatile storage
    configRepo.begin();

    // Initialize kinematics (which attaches and homes servos)
    robot.begin();

    // -------------------------------------------------------------------------
    // 2. NETWORK INITIALIZATION
    // -------------------------------------------------------------------------
    Serial.printf("Connecting to %s\n", WIFI_SSID);
    WiFi.mode(WIFI_STA); // Station mode (connects to your router)
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected! IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.printf("Boot complete! Current Loop Time: %u ms\n", currentLoopTime);

    // Initialize WebUI
    webUI.begin();

    // Initialize OTA
    ArduinoOTA.setHostname("AskalMiniBot");
    ArduinoOTA.onStart([]() {
        Serial.println("Start updating");
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    ArduinoOTA.begin();
}

void loop() {
    unsigned long loopStart = millis();

    // -------------------------------------------------------------------------
    // 1. HANDLE EXTERNAL EVENTS
    // -------------------------------------------------------------------------
    // Process incoming WebSocket messages (joystick inputs, config changes)
    webUI.loop();

    // Handle Over-The-Air (OTA) firmware updates
    ArduinoOTA.handle();

    // -------------------------------------------------------------------------
    // 2. CORE ROBOTICS LOGIC
    // -------------------------------------------------------------------------
    // Calculate new servo angles and execute transitions
    robot.tick();

    // -------------------------------------------------------------------------
    // 3. UI AND LOGGING
    // -------------------------------------------------------------------------
    // Feed latest data to DisplayManager (it actually renders on Core 0 automatically)
#ifdef ENABLE_OLED_DISPLAY
    displayMgr.updateData(WiFi.localIP().toString(), currentLoopTime, robot.getLatestInputs(), robot.getGaitIndex());
#endif
    
    // Serial Log (every 1000ms) for debugging
    unsigned long now = millis();
    if (now - lastSerialLog > 1000) {
        lastSerialLog = now;
        Serial.printf("[System] Loop Time: %u ms\n", currentLoopTime);
    }

    // -------------------------------------------------------------------------
    // 4. TIMING ENFORCEMENT
    // Maintain a consistent execution rate (e.g. 20ms / 50Hz)
    // -------------------------------------------------------------------------
    currentLoopTime = millis() - loopStart;
    int sleepTime = (int)LOOP_DELAY_MS - (int)currentLoopTime;
    if (sleepTime > 0) {
        delay(sleepTime);
    }
    currentLoopTime = millis() - loopStart; // true loop time
}
