/**
 * @file AutoGait.cpp
 * @brief Auto gait implementation: parametrically blends Walk and Gallop by speed.
 */

#include "AutoGait.h"
#include <math.h>
#include <stdlib.h>

// -------------------------------------------------------------------------
// AutoGait Constructor
// -------------------------------------------------------------------------
AutoGait::AutoGait() : phase(0.0f) {}

// -------------------------------------------------------------------------
// AutoGait::reset
// Resets the animation phase so the blended gait starts cleanly when selected.
// -------------------------------------------------------------------------
void AutoGait::reset() {
    phase = 0.0f;
}

// -------------------------------------------------------------------------
// AutoGait::calculate
// Computes servo angles by parametrically blending Walk and Gallop based on
// the commanded speed, giving a smooth low-to-high speed transition.
// -------------------------------------------------------------------------
void AutoGait::calculate(float dt, const JoystickData& inputs, int servoAngles[4]) {
    // -------------------------------------------------------------------------
    // 1. SPEED FACTOR
    // Magnitude of the throttle/yaw vector, normalized and clamped to 0..1.
    // -------------------------------------------------------------------------
    float speed = sqrt(inputs.throttle * inputs.throttle + inputs.yaw * inputs.yaw) / 100.0f;
    if (speed > 1.0f) speed = 1.0f;
    
    // -------------------------------------------------------------------------
    // 2. IDLE CHECK
    // Below 5% speed, stop cycling and hold the Walk stance, allowing the user
    // to tilt the stationary robot with pitch/roll.
    // -------------------------------------------------------------------------
    if (speed < 0.05f) {
        phase = 0.0f;
        int pitchOffset = inputs.pitch * 0.3f;
        int rollOffset = inputs.roll * 0.3f;
        
        // Idle at Walk stance (+20 forward)
        servoAngles[0] = 110 + pitchOffset + rollOffset;
        servoAngles[1] = 110 + pitchOffset - rollOffset;
        servoAngles[2] = 90 - pitchOffset + rollOffset;
        servoAngles[3] = 90 - pitchOffset - rollOffset;
        return;
    }
    
    // -------------------------------------------------------------------------
    // 3. BLEND FACTOR (0.0 = Walk, 1.0 = Gallop)
    // We start transitioning above 0.7 speed, fully gallop at 0.9 speed.
    // The 0.2 divisor is the width of the blend window (0.9 - 0.7).
    // -------------------------------------------------------------------------
    float blendFactor = 0.0f;
    if (speed > 0.7f) {
        blendFactor = (speed - 0.7f) / 0.2f;
    }
    if (blendFactor > 1.0f) blendFactor = 1.0f;
    
    // -------------------------------------------------------------------------
    // 4. PARAMETRIC BLENDING
    // Every gait parameter (phase speed, amplitude, per-leg phase offsets, and
    // base stance) is linearly interpolated between its Walk and Gallop value
    // using blendFactor, so the gait morphs continuously instead of snapping.
    // -------------------------------------------------------------------------
    // Phase speed: Walk is 6, Gallop is 15
    float walkPhaseSpeed = 6.0f;
    float gallopPhaseSpeed = 15.0f;
    float phaseSpeed = (walkPhaseSpeed + (gallopPhaseSpeed - walkPhaseSpeed) * blendFactor) * speed;
    
    if (inputs.throttle < 0) {
        phase -= phaseSpeed * dt;
    } else {
        phase += phaseSpeed * dt;
    }
    
    if (phase > 2 * M_PI) phase -= 2 * M_PI;
    if (phase < 0) phase += 2 * M_PI;
    
    // Leg Amplitudes: Walk is 35, Gallop is 30
    float maxAmp = 35.0f + (30.0f - 35.0f) * blendFactor;
    float leftAmp = inputs.throttle + inputs.yaw;
    float rightAmp = inputs.throttle - inputs.yaw;
    leftAmp = (leftAmp / 100.0f) * maxAmp;
    rightAmp = (rightAmp / 100.0f) * maxAmp;
    
    // Phase Offsets for each leg
    // Walk (4-beat):  FL = 0, FR = PI, HL = 1.5*PI, HR = 0.5*PI
    // Gallop (2-beat): FL = 0, FR = 0,  HL = -0.75*PI (or 1.25*PI), HR = -0.75*PI (or 1.25*PI)
    float walkFL = 0.0f;
    float walkFR = M_PI;
    float walkHL = 1.5f * M_PI;
    float walkHR = 0.5f * M_PI;
    
    float gallopFL = 0.0f;
    float gallopFR = 0.0f;
    float gallopHL = 1.25f * M_PI; // Equivalent to -0.75 * PI within the sin() wave
    float gallopHR = 1.25f * M_PI;
    
    float flPhase = walkFL + (gallopFL - walkFL) * blendFactor;
    float frPhase = walkFR + (gallopFR - walkFR) * blendFactor;
    float hlPhase = walkHL + (gallopHL - walkHL) * blendFactor;
    float hrPhase = walkHR + (gallopHR - walkHR) * blendFactor;
    
    float fl = sin(phase + flPhase);
    float fr = sin(phase + frPhase);
    float hl = sin(phase + hlPhase);
    float hr = sin(phase + hrPhase);
    
    // Base Stance: Walk front is +20, Gallop front is -20
    float walkBaseFront = 20.0f;
    float gallopBaseFront = -20.0f;
    float baseFrontOffset = walkBaseFront + (gallopBaseFront - walkBaseFront) * blendFactor;
    
    // -------------------------------------------------------------------------
    // 5. FINAL ANGLE CALCULATION
    // Apply the blended swing to the blended base stance for each leg.
    // -------------------------------------------------------------------------
    servoAngles[0] = (90 + baseFrontOffset) + fl * leftAmp;
    servoAngles[1] = (90 + baseFrontOffset) + fr * rightAmp;
    servoAngles[2] = 90 + hl * leftAmp;
    servoAngles[3] = 90 + hr * rightAmp;
}
