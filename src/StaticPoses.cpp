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

void WavePose::reset() {
    phase = 0.0f;
}

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

PeePose::PeePose() : elapsedTime(0.0f) {}

void PeePose::reset() {
    elapsedTime = 0.0f;
}

void PeePose::calculate(float dt, const JoystickData& inputs, int servoAngles[4]) {
    int pitchOffset = inputs.pitch * 0.2f;
    int rollOffset = inputs.roll * 0.2f;

    elapsedTime += dt;

    if (elapsedTime < 4.0f) {
        // Front legs planted
        servoAngles[0] = 90 + pitchOffset + rollOffset;
        servoAngles[1] = 90 + pitchOffset - rollOffset;
        // Hind left planted
        servoAngles[2] = 90 - pitchOffset + rollOffset;
        
        // Hind right swings backward and oscillates up and down
        int wag = sin(elapsedTime * 15.0f) * 10;
        servoAngles[3] = 130 + wag - pitchOffset - rollOffset; // Swing backwards (to 130 degrees) and wag
    } else {
        // Return to normal standing
        servoAngles[0] = 90 + pitchOffset + rollOffset;
        servoAngles[1] = 90 + pitchOffset - rollOffset;
        servoAngles[2] = 90 - pitchOffset + rollOffset;
        servoAngles[3] = 90 - pitchOffset - rollOffset;
    }
}

ScrapePose::ScrapePose() : phase(0.0f) {}

void ScrapePose::calculate(float dt, const JoystickData& inputs, int servoAngles[4]) {
    int pitchOffset = inputs.pitch * 0.2f;
    int rollOffset = inputs.roll * 0.2f;

    phase += 15.0f * dt; // Fast sweeping speed
    if (phase > 2 * 3.14159f) phase -= 2 * 3.14159f;

    // We want a sawtooth-like or fast sweep backward, slow forward.
    // A sine wave is okay for now, we just make the amplitude large.
    // Hind servo backwards is a negative angle.
    int sweepAngleLeft = sin(phase) * 30; // +/- 30 degrees
    int sweepAngleRight = -sin(phase) * 30; // opposite phase to alternate

    // Plant front legs
    servoAngles[0] = 60 + pitchOffset + rollOffset;
    servoAngles[1] = 60 + pitchOffset - rollOffset;
    
    // Sweep hind legs
    servoAngles[2] = 90 + sweepAngleLeft - pitchOffset + rollOffset;
    servoAngles[3] = 90 + sweepAngleRight - pitchOffset - rollOffset;
}

void InfoPose::calculate(float dt, const JoystickData& inputs, int servoAngles[4]) {
    // Just sit quietly while showing weather
    int pitchOffset = inputs.pitch * 0.2f;
    int rollOffset = inputs.roll * 0.2f;
    
    servoAngles[0] = 90 + pitchOffset + rollOffset;
    servoAngles[1] = 90 + pitchOffset - rollOffset;
    servoAngles[2] = 30 - pitchOffset + rollOffset;
    servoAngles[3] = 30 - pitchOffset - rollOffset;
}
