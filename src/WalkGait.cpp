#include "WalkGait.h"
#include <math.h>
#include <stdlib.h>

// -------------------------------------------------------------------------
// WalkGait Constructor
// Initializes the animation phase to 0.
// -------------------------------------------------------------------------
WalkGait::WalkGait() : phase(0.0f) {}

// -------------------------------------------------------------------------
// WalkGait::calculate
// Computes servo angles for a 4-beat walking motion.
// -------------------------------------------------------------------------
void WalkGait::calculate(float dt, const JoystickData& inputs, int servoAngles[4]) {
    // -------------------------------------------------------------------------
    // 1. IDLE CHECK
    // Stop walking if joystick inputs are neutral. Reset the phase and 
    // allow the user to tilt the stationary robot using pitch and roll.
    // -------------------------------------------------------------------------
    if (abs(inputs.throttle) < 5.0f && abs(inputs.yaw) < 5.0f) {
        phase = 0.0f;
        int pitchOffset = inputs.pitch * 0.3f;
        int rollOffset = inputs.roll * 0.3f;
        servoAngles[0] = 110 + pitchOffset + rollOffset;
        servoAngles[1] = 110 + pitchOffset - rollOffset;
        servoAngles[2] = 90 - pitchOffset + rollOffset;
        servoAngles[3] = 90 - pitchOffset - rollOffset;
        return;
    }
    
    // -------------------------------------------------------------------------
    // 2. SPEED AND PHASE CALCULATION
    // Calculate movement speed and update the animation phase.
    // Walking is a slow gait, so the phase multiplier is small (6.0f).
    // -------------------------------------------------------------------------
    float speed = sqrt(inputs.throttle * inputs.throttle + inputs.yaw * inputs.yaw) / 100.0f;
    if (speed > 1.0f) speed = 1.0f;
    
    float phaseSpeed = 6.0f * speed; // Slower than trot
    
    // Reverse animation direction for backward movement
    if (inputs.throttle < 0) {
        phase -= phaseSpeed * dt;
    } else {
        phase += phaseSpeed * dt;
    }
    
    // Wrap phase between 0 and 2*PI
    if (phase > 2 * M_PI) phase -= 2 * M_PI;
    if (phase < 0) phase += 2 * M_PI;
    
    // -------------------------------------------------------------------------
    // 3. LEG SWING AMPLITUDES
    // Combine throttle and yaw for steering. 
    // maxAmp limits how far the legs can swing forward/backward (35 degrees).
    // -------------------------------------------------------------------------
    float leftAmp = inputs.throttle + inputs.yaw;
    float rightAmp = inputs.throttle - inputs.yaw;
    float maxAmp = 35.0f; 
    
    leftAmp = (leftAmp / 100.0f) * maxAmp;
    rightAmp = (rightAmp / 100.0f) * maxAmp;
    
    // -------------------------------------------------------------------------
    // 4. WALK (4-BEAT) LOGIC
    // A stable walk moves one leg at a time. This is achieved by offsetting 
    // the sine wave phase of each leg by PI/2 (90 degrees).
    // Sequence roughly mimics: Front Left -> Hind Right -> Front Right -> Hind Left
    // -------------------------------------------------------------------------
    float lf = sin(phase);
    float hr = sin(phase + M_PI / 2);
    float rf = sin(phase + M_PI);
    float hl = sin(phase + 3 * M_PI / 2);
    
    // -------------------------------------------------------------------------
    // 5. FINAL ANGLE CALCULATION
    // Apply the swings to the base 90 degree angle.
    // -------------------------------------------------------------------------
    servoAngles[0] = (90 + 20) + lf * leftAmp;
    servoAngles[1] = (90 + 20) + rf * rightAmp;
    servoAngles[2] = 90 + hl * leftAmp;
    servoAngles[3] = 90 + hr * rightAmp;
}
