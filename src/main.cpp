/**
 * @file main.cpp
 * @brief Application entry point and composition root for AskalMiniBot.
 *
 * This file wires the SOLID-decomposed modules together using constructor
 * injection: it instantiates every concrete implementation, assembles the
 * ordered array of gaits/poses, and injects the dependencies into the
 * high-level RobotKinematics and WebUIManager. It then runs hardware/network
 * init in setup() and the fixed-rate control loop in loop().
 */

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

// -------------------------------------------------------------------------
// LEAF DEPENDENCIES
// Concrete implementations of the storage and hardware interfaces. These have
// no dependencies on the higher-level components and are injected into them.
// -------------------------------------------------------------------------
PreferencesConfig* configRepo = nullptr; ///< NVS-backed configuration store.
HardwareServoDriver servoDriver;  ///< ESP32Servo-backed physical servo driver.

// -------------------------------------------------------------------------
// GAIT & POSE STRATEGIES
// One instance per IGaitStrategy implementation. Order here defines the index
// used by the Web UI (set_mode) and getGaitIndex(); keep allGaits[] in sync.
// -------------------------------------------------------------------------
TrotGait trotGait;
WalkGait walkGait;
GallopGait gallopGait;
AutoGait autoGait;
SitPose sitPose;
StretchPose stretchPose;
FoldPose foldPose;
WavePose wavePose;
PeePose peePose;
ScrapePose scrapePose;
InfoPose infoPose;

/// Ordered registry of all selectable gaits/poses. Index = Web UI mode number:
/// 0 Trot, 1 Walk, 2 Gallop, 3 Auto, 4 Sit, 5 Stretch, 6 Fold, 7 Wave, 8 Pee, 9 Scrape, 10 Info.
IGaitStrategy* allGaits[] = { &trotGait, &walkGait, &gallopGait, &autoGait, &sitPose, &stretchPose, &foldPose, &wavePose, &peePose, &scrapePose, &infoPose };
int numGaits = sizeof(allGaits) / sizeof(allGaits[0]); ///< Number of entries in allGaits[].

// -------------------------------------------------------------------------
// HIGH-LEVEL COMPONENTS
// Constructed with their dependencies injected (composition root pattern).
// -------------------------------------------------------------------------
RobotKinematics* robot = nullptr; ///< Core motion engine.
WebUIManager* webUI = nullptr;    ///< WiFi web/WebSocket controller.
#ifdef ENABLE_OLED_DISPLAY
DisplayManager* displayMgr = nullptr; ///< Optional OLED (Core 0 task).
#endif

unsigned long lastOledUpdate = 0; ///< Reserved: timestamp of last OLED refresh.
unsigned long lastLoopTick = 0;   ///< Reserved: timestamp of last loop tick.
unsigned long lastSerialLog = 0;  ///< Timestamp of the last 1Hz serial status log.
uint32_t currentLoopTime = 0;     ///< Measured execution time of the last loop iteration (ms).

// -------------------------------------------------------------------------
// setup
// One-time boot sequence: brings up hardware (display, config, servos), then
// the network (WiFi STA), then the web UI and OTA update services.
// -------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    // Add small delay to allow serial to connect via USB CDC if needed
    delay(2000); 
    Serial.println("\nAskalMiniBot Starting...");

    configRepo = new PreferencesConfig();

    // Initialize display early to show boot status
#ifdef ENABLE_OLED_DISPLAY
    displayMgr = new DisplayManager(*configRepo);
    displayMgr->begin();
#endif

    // Load saved servo calibration and API keys from non-volatile storage
    configRepo->begin();

    // Initialize kinematics (which attaches and homes servos)
    robot = new RobotKinematics(servoDriver, *configRepo, allGaits, numGaits);
    robot->begin();

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
    webUI = new WebUIManager(*robot, *configRepo);
    webUI->begin();

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

// -------------------------------------------------------------------------
// loop
// Fixed-rate control loop (~50Hz). Each iteration services external events,
// advances the kinematics, refreshes UI/logging, then sleeps to hold cadence.
// -------------------------------------------------------------------------
void loop() {
    unsigned long loopStart = millis();

    // -------------------------------------------------------------------------
    // 1. HANDLE EXTERNAL EVENTS
    // -------------------------------------------------------------------------
    // Process incoming WebSocket messages (joystick inputs, config changes)
    webUI->loop();

    // Handle Over-The-Air (OTA) firmware updates
    ArduinoOTA.handle();

    // -------------------------------------------------------------------------
    // 2. CORE ROBOTICS LOGIC
    // -------------------------------------------------------------------------
    // Calculate new servo angles and execute transitions
    robot->tick();

    // -------------------------------------------------------------------------
    // 3. UI AND LOGGING
    // -------------------------------------------------------------------------
    // Feed latest data to DisplayManager (it actually renders on Core 0 automatically)
#ifdef ENABLE_OLED_DISPLAY
    displayMgr->updateData(WiFi.localIP().toString(), currentLoopTime, robot->getLatestInputs(), robot->getGaitIndex());
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
