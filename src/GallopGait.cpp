#include "GallopGait.h"
#include <math.h>
#include <stdlib.h>

// -------------------------------------------------------------------------
// GallopGait Constructor
// Initializes the animation phase to 0.
// -------------------------------------------------------------------------
GallopGait::GallopGait() : phase(0.0f) {}

// -------------------------------------------------------------------------
// GallopGait::calculate
// Computes servo angles for a galloping motion based on joystick inputs.
// -------------------------------------------------------------------------
void GallopGait::calculate(float dt, const JoystickData& inputs, int servoAngles[4]) {
    // -------------------------------------------------------------------------
    // 1. IDLE CHECK
    // If there is very little joystick input (throttle and yaw are near zero),
    // we stop the gallop animation, reset the phase, and transition to a standing 
    // pose. We still allow pitch and roll inputs to tilt the stationary robot.
    // -------------------------------------------------------------------------
    if (abs(inputs.throttle) < 5.0f && abs(inputs.yaw) < 5.0f) {
        phase = 0.0f; // Reset animation cycle
        int pitchOffset = inputs.pitch * 0.3f;
        int rollOffset = inputs.roll * 0.3f;
        
        // Base standing angle is 90. Apply tilt offsets.
        servoAngles[0] = 90 + pitchOffset + rollOffset;
        servoAngles[1] = 90 + pitchOffset - rollOffset;
        servoAngles[2] = 90 - pitchOffset + rollOffset;
        servoAngles[3] = 90 - pitchOffset - rollOffset;
        return;
    }
    
    // -------------------------------------------------------------------------
    // 2. SPEED AND PHASE CALCULATION
    // Calculate the magnitude of the joystick vector (speed).
    // The phase determines where we are in the cyclic animation (like a sine wave).
    // Galloping is intentionally faster than trotting (multiplier 15.0f).
    // -------------------------------------------------------------------------
    float speed = sqrt(inputs.throttle * inputs.throttle + inputs.yaw * inputs.yaw) / 100.0f;
    if (speed > 1.0f) speed = 1.0f; // Clamp maximum speed to 1.0 (100%)
    
    float phaseSpeed = 15.0f * speed; // How fast the legs cycle
    
    // Reverse the animation direction if moving backwards
    if (inputs.throttle < 0) {
        phase -= phaseSpeed * dt;
    } else {
        phase += phaseSpeed * dt;
    }
    
    // Keep phase safely wrapped between 0 and 2*PI (a full circle)
    if (phase > 2 * M_PI) phase -= 2 * M_PI;
    if (phase < 0) phase += 2 * M_PI;
    
    // -------------------------------------------------------------------------
    // 3. LEG SWING AMPLITUDES
    // We combine the forward/backward throttle with left/right yaw to determine
    // how much each side of the robot should swing its legs. This creates steering.
    // -------------------------------------------------------------------------
    float leftAmp = inputs.throttle + inputs.yaw;
    float rightAmp = inputs.throttle - inputs.yaw;
    float maxAmp = 45.0f; 
    
    leftAmp = (leftAmp / 100.0f) * maxAmp;
    rightAmp = (rightAmp / 100.0f) * maxAmp;
    
    // -------------------------------------------------------------------------
    // 4. BOUNDING (GALLOP) LOGIC
    // In a gallop/bound, both front legs move exactly in sync (using sin(phase)),
    // and both hind legs move exactly in sync but opposite to the front legs 
    // (using sin(phase + PI)). This creates a rocking/bounding forward motion.
    // -------------------------------------------------------------------------
    float front = sin(phase);
    float hind = sin(phase + M_PI);
    
    // -------------------------------------------------------------------------
    // 5. FINAL ANGLE CALCULATION
    // We multiply the base leg swing (-1.0 to 1.0) by the side amplitude (leftAmp/rightAmp)
    // to get the final servo angle deviation from 90 (center).
    // -------------------------------------------------------------------------
    servoAngles[0] = 90 + front * leftAmp;
    servoAngles[1] = 90 + front * rightAmp;
    servoAngles[2] = 90 + hind * leftAmp;
    servoAngles[3] = 90 + hind * rightAmp;
}
