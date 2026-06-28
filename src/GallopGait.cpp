#include "GallopGait.h"
#include <math.h>
#include <stdlib.h>

GallopGait::GallopGait() : phase(0.0f) {}

void GallopGait::calculate(float dt, const JoystickData& inputs, int servoAngles[4]) {
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
    
    float phaseSpeed = 15.0f * speed; // Faster than trot
    if (inputs.throttle < 0) {
        phase -= phaseSpeed * dt;
    } else {
        phase += phaseSpeed * dt;
    }
    
    if (phase > 2 * M_PI) phase -= 2 * M_PI;
    if (phase < 0) phase += 2 * M_PI;
    
    float leftAmp = inputs.throttle + inputs.yaw;
    float rightAmp = inputs.throttle - inputs.yaw;
    float maxAmp = 45.0f; 
    
    leftAmp = (leftAmp / 100.0f) * maxAmp;
    rightAmp = (rightAmp / 100.0f) * maxAmp;
    
    // Gallop: Front legs together, hind legs together
    float front = sin(phase);
    float hind = sin(phase + M_PI);
    
    servoAngles[0] = 90 + front * leftAmp;
    servoAngles[1] = 90 + front * rightAmp;
    servoAngles[2] = 90 + hind * leftAmp;
    servoAngles[3] = 90 + hind * rightAmp;
}
