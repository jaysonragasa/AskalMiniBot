#include "WalkGait.h"
#include <math.h>
#include <stdlib.h>

WalkGait::WalkGait() : phase(0.0f) {}

void WalkGait::calculate(float dt, const JoystickData& inputs, int servoAngles[4]) {
    if (abs(inputs.throttle) < 5.0f && abs(inputs.yaw) < 5.0f) {
        phase = 0.0f;
        int pitchOffset = inputs.pitch * 0.3f;
        int rollOffset = inputs.roll * 0.3f;
        servoAngles[0] = 90 + pitchOffset + rollOffset;
        servoAngles[1] = 90 + pitchOffset - rollOffset;
        servoAngles[2] = 90 - pitchOffset + rollOffset;
        servoAngles[3] = 90 - pitchOffset - rollOffset;
        return;
    }
    
    float speed = sqrt(inputs.throttle * inputs.throttle + inputs.yaw * inputs.yaw) / 100.0f;
    if (speed > 1.0f) speed = 1.0f;
    
    float phaseSpeed = 6.0f * speed; // Slower than trot
    if (inputs.throttle < 0) {
        phase -= phaseSpeed * dt;
    } else {
        phase += phaseSpeed * dt;
    }
    
    if (phase > 2 * M_PI) phase -= 2 * M_PI;
    if (phase < 0) phase += 2 * M_PI;
    
    float leftAmp = inputs.throttle + inputs.yaw;
    float rightAmp = inputs.throttle - inputs.yaw;
    float maxAmp = 35.0f; 
    
    leftAmp = (leftAmp / 100.0f) * maxAmp;
    rightAmp = (rightAmp / 100.0f) * maxAmp;
    
    // Walk: 4-beat sequence
    float lf = sin(phase);
    float rh = sin(phase + M_PI/2);
    float rf = sin(phase + M_PI);
    float lh = sin(phase + 3*M_PI/2);
    
    servoAngles[0] = 90 + lf * leftAmp;
    servoAngles[1] = 90 + rf * rightAmp;
    servoAngles[2] = 90 + lh * leftAmp;
    servoAngles[3] = 90 + rh * rightAmp;
}
