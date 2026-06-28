#include "TrotGait.h"
#include <math.h>
#include <stdlib.h>

TrotGait::TrotGait() : phase(0.0f) {}

void TrotGait::calculate(float dt, const JoystickData& inputs, int servoAngles[4]) {
    // If joystick is idle, phase resets to 0 (standing)
    if (abs(inputs.throttle) < 5.0f && abs(inputs.yaw) < 5.0f) {
        phase = 0.0f;
        // Stand still with pitch/roll applied as offsets
        int pitchOffset = inputs.pitch * 0.3f;
        int rollOffset = inputs.roll * 0.3f;
        
        servoAngles[0] = 90 + pitchOffset + rollOffset;
        servoAngles[1] = 90 + pitchOffset - rollOffset;
        servoAngles[2] = 90 - pitchOffset + rollOffset;
        servoAngles[3] = 90 - pitchOffset - rollOffset;
        return;
    }
    
    // Determine speed from throttle and yaw
    float speed = sqrt(inputs.throttle * inputs.throttle + inputs.yaw * inputs.yaw) / 100.0f;
    if (speed > 1.0f) speed = 1.0f;
    
    // Advance phase
    float phaseSpeed = 10.0f * speed; 
    if (inputs.throttle < 0) {
        phase -= phaseSpeed * dt;
    } else {
        phase += phaseSpeed * dt;
    }
    
    if (phase > 2 * M_PI) phase -= 2 * M_PI;
    if (phase < 0) phase += 2 * M_PI;
    
    // Calculate amplitudes based on yaw for steering (differential drive logic)
    float leftAmp = inputs.throttle + inputs.yaw;
    float rightAmp = inputs.throttle - inputs.yaw;
    
    // Normalize
    float maxAmp = 40.0f; // Max angle deviation from 90
    leftAmp = (leftAmp / 100.0f) * maxAmp;
    rightAmp = (rightAmp / 100.0f) * maxAmp;
    
    // Trot: Diagonal pairs move together. 0 & 3, 1 & 2.
    // 0 = Front Left, 1 = Front Right, 2 = Back Left, 3 = Back Right
    float pair1 = sin(phase);
    float pair2 = sin(phase + M_PI); // 180 degrees out of phase
    
    servoAngles[0] = 90 + pair1 * leftAmp;
    servoAngles[1] = 90 + pair2 * rightAmp;
    servoAngles[2] = 90 + pair2 * leftAmp;
    servoAngles[3] = 90 + pair1 * rightAmp;
}
