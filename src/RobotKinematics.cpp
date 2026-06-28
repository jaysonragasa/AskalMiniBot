#include "RobotKinematics.h"
#include <Arduino.h>

RobotKinematics::RobotKinematics(IServoDriver& servoDriver, IConfigRepository& configRepo, IGaitStrategy** gaitsArray, int count)
    : driver(servoDriver), config(configRepo), gaits(gaitsArray), numGaits(count) {
    currentGait = (count > 0) ? gaits[0] : nullptr;
    latestInputs = {0.0f, 0.0f, 0.0f, 0.0f};
    lastTick = 0;
}

void RobotKinematics::begin() {
    driver.begin();
    reattachServos();
    lastTick = millis();
}

void RobotKinematics::onConfigUpdated() {
    reattachServos();
}

void RobotKinematics::setGait(int index) {
    if (index >= 0 && index < numGaits) {
        currentGait = gaits[index];
    }
}

void RobotKinematics::reattachServos() {
    for (int i = 0; i < 4; i++) {
        if (driver.isAttached(i)) {
            driver.detach(i);
        }
        ServoConfig cfg = config.getServoConfig(i);
        driver.attach(i, cfg.pin);
        
        // Go to home position
        driver.write(i, processAngle(i, 90));
        
        // Stagger servo initialization to prevent massive power spikes (brownouts)
        delay(300);
    }
}

void RobotKinematics::onJoystickUpdate(float throttle, float yaw, float pitch, float roll) {
    latestInputs.throttle = throttle;
    latestInputs.yaw = yaw;
    latestInputs.pitch = pitch;
    latestInputs.roll = roll;
}

int RobotKinematics::processAngle(int servoIndex, int targetAngle, bool applyLimits) {
    ServoConfig cfg = config.getServoConfig(servoIndex);
    
    if (!cfg.enabled) {
        return 90; // Just stay at center if disabled
    }
    
    // Apply offset and invert multiplier (target is relative to 90 degrees)
    int deviation = targetAngle - 90;
    deviation *= cfg.invert; // 1 for normal, -1 for inverted
    
    int finalAngle = 90 + deviation + (int)cfg.offset;
    
    if (applyLimits) {
        // Constrain to configured maxAngle limits (relative to center 90)
        int minSafe = 90 - (int)cfg.maxAngle;
        int maxSafe = 90 + (int)cfg.maxAngle;
        
        if (finalAngle < minSafe) finalAngle = minSafe;
        if (finalAngle > maxSafe) finalAngle = maxSafe;
    }
    
    return finalAngle;
}

void RobotKinematics::tick() {
    unsigned long now = millis();
    float dt = (now - lastTick) / 1000.0f;
    lastTick = now;
    
    if (currentGait) {
        int targetAngles[4] = {90, 90, 90, 90};
        currentGait->calculate(dt, latestInputs, targetAngles);
        
        bool useLimits = currentGait->applyLimits();
        for (int i = 0; i < 4; i++) {
            driver.write(i, processAngle(i, targetAngles[i], useLimits));
        }
    }
}
