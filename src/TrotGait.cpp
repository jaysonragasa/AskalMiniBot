/**
 * @file TrotGait.cpp
 * @brief Trot gait implementation: diagonal leg pairs move together, 180 deg apart.
 */

#include "TrotGait.h"
#include <math.h>
#include <stdlib.h>

// -------------------------------------------------------------------------
// TrotGait Constructor
// Initializes the animation phase to 0.
// -------------------------------------------------------------------------
TrotGait::TrotGait() : phase(0.0f) {}

// -------------------------------------------------------------------------
// TrotGait::calculate
// Computes servo angles for a diagonal trotting motion.
// -------------------------------------------------------------------------
void TrotGait::calculate(float dt, const JoystickData& inputs, int servoAngles[4]) {
    // -------------------------------------------------------------------------
    // 1. IDLE CHECK
    // If joystick is idle, phase resets to 0 (standing). We allow pitch/roll 
    // to tilt the robot while standing still.
    // -------------------------------------------------------------------------
    if (abs(inputs.throttle) < 5.0f && abs(inputs.yaw) < 5.0f) {
        phase = 0.0f;
        int pitchOffset = inputs.pitch * 0.3f;
        int rollOffset = inputs.roll * 0.3f;
        
        // Base standing angle for Trot includes the +20 front offset.
        servoAngles[0] = 110 + pitchOffset + rollOffset;
        servoAngles[1] = 110 + pitchOffset - rollOffset;
        servoAngles[2] = 90 - pitchOffset + rollOffset;
        servoAngles[3] = 90 - pitchOffset - rollOffset;
        return;
    }
    
    // -------------------------------------------------------------------------
    // 2. SPEED AND PHASE CALCULATION
    // Calculate speed magnitude and advance the phase.
    // Trot is a medium speed gait (multiplier 10.0f).
    // -------------------------------------------------------------------------
    float speed = sqrt(inputs.throttle * inputs.throttle + inputs.yaw * inputs.yaw) / 100.0f;
    if (speed > 1.0f) speed = 1.0f;
    
    float phaseSpeed = 10.0f * speed; 
    
    // Reverse animation direction for backward movement
    if (inputs.throttle < 0) {
        phase -= phaseSpeed * dt;
    } else {
        phase += phaseSpeed * dt;
    }
    
    if (phase > 2 * M_PI) phase -= 2 * M_PI;
    if (phase < 0) phase += 2 * M_PI;
    
    // -------------------------------------------------------------------------
    // 3. LEG SWING AMPLITUDES (STEERING)
    // Calculate amplitudes based on yaw for steering (differential drive logic).
    // maxAmp limits the maximum leg swing to 40 degrees.
    // -------------------------------------------------------------------------
    float leftAmp = inputs.throttle + inputs.yaw;
    float rightAmp = inputs.throttle - inputs.yaw;
    
    float maxAmp = 40.0f; 
    leftAmp = (leftAmp / 100.0f) * maxAmp;
    rightAmp = (rightAmp / 100.0f) * maxAmp;
    
    // -------------------------------------------------------------------------
    // 4. TROT LOGIC
    // In a trot, diagonal pairs of legs move exactly together.
    // Pair 1: Front Left (0) & Back Right (3)
    // Pair 2: Front Right (1) & Back Left (2)
    // Pair 2 is exactly 180 degrees (PI) out of phase with Pair 1.
    // -------------------------------------------------------------------------
    float pair1 = sin(phase);
    float pair2 = sin(phase + M_PI);
    
    // -------------------------------------------------------------------------
    // 5. FINAL ANGLE CALCULATION
    // -------------------------------------------------------------------------
    servoAngles[0] = (90 + 20) + pair1 * leftAmp;  // Front Left
    servoAngles[1] = (90 + 20) + pair2 * rightAmp; // Front Right
    servoAngles[2] = 90 + pair2 * leftAmp;  // Back Left
    servoAngles[3] = 90 + pair1 * rightAmp; // Back Right
}
