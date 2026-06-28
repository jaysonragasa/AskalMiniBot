#include "StaticPoses.h"
#include <math.h>
void SitPose::calculate(float dt, const JoystickData& inputs, int servoAngles[4]) {
    // Apply slight pitch/roll joystick tweaking if desired
    int pitchOffset = inputs.pitch * 0.2f;
    int rollOffset = inputs.roll * 0.2f;
    
    // Front legs straight down, hind legs tucked forward under the belly
    servoAngles[0] = 90 + pitchOffset + rollOffset;
    servoAngles[1] = 90 + pitchOffset - rollOffset;
    servoAngles[2] = 30 - pitchOffset + rollOffset;
    servoAngles[3] = 30 - pitchOffset - rollOffset;
}

void StretchPose::calculate(float dt, const JoystickData& inputs, int servoAngles[4]) {
    // Apply slight pitch/roll joystick tweaking if desired
    int pitchOffset = inputs.pitch * 0.2f;
    int rollOffset = inputs.roll * 0.2f;
    
    // Front legs stretched forward, hind legs stretched backward (superman)
    servoAngles[0] = 30 + pitchOffset + rollOffset;
    servoAngles[1] = 30 + pitchOffset - rollOffset;
    servoAngles[2] = 150 - pitchOffset + rollOffset;
    servoAngles[3] = 150 - pitchOffset - rollOffset;
}

WavePose::WavePose() : phase(0.0f) {}

void WavePose::calculate(float dt, const JoystickData& inputs, int servoAngles[4]) {
    int pitchOffset = inputs.pitch * 0.2f;
    int rollOffset = inputs.roll * 0.2f;

    phase += 15.0f * dt; // Wave speed
    if (phase > 2 * 3.14159f) phase -= 2 * 3.14159f;
    
    int waveAngle = sin(phase) * 35; // 35 degrees amplitude
    
    // Front-Left: Planted (sit)
    servoAngles[0] = 90 + pitchOffset + rollOffset;
    // Front-Right: Lifted up and waving
    servoAngles[1] = 20 + waveAngle + pitchOffset - rollOffset;
    
    // Hind legs: Tucked under (sit)
    servoAngles[2] = 30 - pitchOffset + rollOffset;
    servoAngles[3] = 30 - pitchOffset - rollOffset;
}
