#include "RobotKinematics.h"
#include <Arduino.h>

RobotKinematics::RobotKinematics(IServoDriver& servoDriver, IConfigRepository& configRepo, IGaitStrategy** gaitsArray, int count)
    : driver(servoDriver), config(configRepo), gaits(gaitsArray), numGaits(count) {
    currentGait = (count > 0) ? gaits[0] : nullptr;
    currentGaitIndex = (count > 0) ? 0 : -1;
    latestInputs = {0.0f, 0.0f, 0.0f, 0.0f};
    lastTick = 0;
    transitionProgress = 0.0f;
    pendingGaitIndex = -1;
    
    if (currentGait) {
        currentGait->reset();
    }
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
        if (currentGaitIndex != index && pendingGaitIndex != index) {
            // -------------------------------------------------------------------------
            // FOLDED POSE TRANSITION INTERCEPT
            // If the robot is sitting (hind legs tucked under) and is instructed to 
            // stand up, it will physically flip backwards due to its center of mass.
            // We intercept this here. Instead of immediately switching, we set a 
            // `pendingGaitIndex` and trigger a `transitionProgress` state machine 
            // inside tick() to gracefully stretch the legs first.
            // -------------------------------------------------------------------------
            if (currentGait && currentGait->isFoldedPose() && !gaits[index]->isFoldedPose()) {
                pendingGaitIndex = index;
                transitionProgress = 0.01f; // Kickstart the transition in tick()
            } else {
                // Normal immediate switch
                currentGait = gaits[index];
                currentGaitIndex = index;
                if (currentGait) {
                    currentGait->reset();
                }
                transitionProgress = 0.0f;
                pendingGaitIndex = -1;
            }
        }
    }
}

int RobotKinematics::getGaitIndex() const {
    if (transitionProgress > 0.0f) return pendingGaitIndex;
    return currentGaitIndex;
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
        return 90; // Just stay at mechanical center if disabled in WebUI
    }
    
    // -------------------------------------------------------------------------
    // KINEMATIC MAPPING
    // 1. Calculate deviation from the logical center (90 degrees).
    // 2. Invert it if the user calibrated this specific servo as inverted.
    // 3. Re-apply the center (90) and add the user's manual trim offset.
    // -------------------------------------------------------------------------
    int deviation = targetAngle - 90;
    deviation *= cfg.invert; // 1 for normal, -1 for inverted
    
    int finalAngle = 90 + deviation + (int)cfg.offset;
    
    // -------------------------------------------------------------------------
    // SAFETY LIMITS
    // Prevent the servos from crashing into the chassis or binding.
    // Limits are applied dynamically (Static poses usually ignore limits to 
    // allow extreme folding, while walking/trotting strictly obeys them).
    // -------------------------------------------------------------------------
    if (applyLimits) {
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
    
    // -------------------------------------------------------------------------
    // SMOOTH STRETCH TRANSITION STATE MACHINE
    // If a transition was triggered in setGait(), this block executes exclusively.
    // It interpolates the leg angles mathematically to prevent the robot from 
    // abruptly jerking and flipping backward.
    // -------------------------------------------------------------------------
    if (transitionProgress > 0.0f) {
        // Advance progress based on delta time (completes in ~0.66 seconds)
        transitionProgress += dt * 1.5f; 
        
        if (transitionProgress >= 1.0f) {
            // The animation has finished. We can now safely apply the new gait.
            currentGait = gaits[pendingGaitIndex];
            currentGaitIndex = pendingGaitIndex;
            if (currentGait) currentGait->reset();
            
            transitionProgress = 0.0f;
            pendingGaitIndex = -1;
        } else {
            // Front legs instantly stretch forward to anchor the center of mass
            int frontAngle = 30;
            
            // Hind legs slowly interpolate from folded (30) to stretched back (150)
            int hindStart = 30;
            int hindEnd = 150;
            int hindAngle = hindStart + (hindEnd - hindStart) * transitionProgress;
            
            driver.write(0, processAngle(0, frontAngle, false));
            driver.write(1, processAngle(1, frontAngle, false));
            driver.write(2, processAngle(2, hindAngle, false));
            driver.write(3, processAngle(3, hindAngle, false));
            
            // Return immediately. We don't want the normal gait calculations 
            // overwriting our transition angles.
            return; 
        }
    }
    
    if (currentGait) {
        int targetAngles[4] = {90, 90, 90, 90};
        currentGait->calculate(dt, latestInputs, targetAngles);
        
        // --- Pitch Kinematic Overrides ---
        // Empirical observation: DECREASING angle (< 90) moves leg BACKWARD.
        if (abs(latestInputs.pitch) > 5.0f && abs(latestInputs.throttle) < 5.0f && abs(latestInputs.yaw) < 5.0f) 
        {
            float rollOffset = latestInputs.roll * 0.3f;
            float magnitude = ((int)latestInputs.pitch) / 100.0f;
            
            // Note: DECREASING angle (- 30) moves the leg BACKWARD.
            if (latestInputs.pitch > 5.0f) {
                // PITCH UP (Joystick pushed UP -> Positive)
                // Hind backward heavily (-30), Front neutral (90)

                targetAngles[0] = 90 + rollOffset; // Front Left
                targetAngles[1] = 90 - rollOffset; // Front Right
                targetAngles[2] = 90 - (30 * magnitude) + rollOffset; // Hind Left
                targetAngles[3] = 90 - (30 * magnitude) - rollOffset; // Hind Right
            } 
            else if (latestInputs.pitch < -5.0f) {
                // PITCH DOWN (Joystick pulled DOWN -> Negative)
                // Front backward heavily (-30), Hind backward slightly (-15)
                magnitude = -magnitude;

                targetAngles[0] = 90 + (30 * magnitude) + rollOffset; // Front Left
                targetAngles[1] = 90 + (30 * magnitude) - rollOffset; // Front Right
                targetAngles[2] = 90 + (15 * magnitude) + rollOffset; // Hind Left
                targetAngles[3] = 90 + (15 * magnitude) - rollOffset; // Hind Right
            }
        }
        
        // Determine if limits should be applied
        // If there's an active pitch override, we temporarily disable limits
        // Otherwise, we respect what the current pose/gait requests
        bool useLimits = currentGait->applyLimits(); 
        if (abs(latestInputs.pitch) >= 5.0f || abs(latestInputs.roll) >= 5.0f) {
            useLimits = false;
        }
        
        for (int i = 0; i < 4; i++) {
            driver.write(i, processAngle(i, targetAngles[i], useLimits));
        }
    }
}
