#include "StaticPoses.h"
#include <math.h>
// -------------------------------------------------------------------------
// SitPose::calculate
// Computes the servo angles for sitting.
// -------------------------------------------------------------------------
void SitPose::calculate(float dt, const JoystickData& inputs, int servoAngles[4]) {
    // -------------------------------------------------------------------------
    // The SitPose folds the hind legs completely underneath the robot (angle 30)
    // while keeping the front legs straight down (angle 90).
    // Joystick pitch and roll can still be used to dynamically tilt the robot.
    // -------------------------------------------------------------------------
    
    // Calculate slight pitch/roll deviations (scaled down by 0.2f for subtlety)
    int pitchOffset = inputs.pitch * 0.2f;
    int rollOffset = inputs.roll * 0.2f;
    
    // Apply the base pose + joystick offsets
    servoAngles[0] = 90 + pitchOffset + rollOffset;  // Front Left
    servoAngles[1] = 90 + pitchOffset - rollOffset;  // Front Right
    servoAngles[2] = 30 - pitchOffset + rollOffset;  // Hind Left (Folded)
    servoAngles[3] = 30 - pitchOffset - rollOffset;  // Hind Right (Folded)
}

// -------------------------------------------------------------------------
// StretchPose::calculate
// Computes the servo angles for a stretched-out posture.
// -------------------------------------------------------------------------
void StretchPose::calculate(float dt, const JoystickData& inputs, int servoAngles[4]) {
    // -------------------------------------------------------------------------
    // The StretchPose extends all legs outward. Front legs point forward (30),
    // and hind legs point backward (150). This lowers the robot's center of mass.
    // -------------------------------------------------------------------------
    int pitchOffset = inputs.pitch * 0.2f;
    int rollOffset = inputs.roll * 0.2f;
    
    servoAngles[0] = 30 + pitchOffset + rollOffset;  // Front Left (Forward)
    servoAngles[1] = 30 + pitchOffset - rollOffset;  // Front Right (Forward)
    servoAngles[2] = 150 - pitchOffset + rollOffset; // Hind Left (Backward)
    servoAngles[3] = 150 - pitchOffset - rollOffset; // Hind Right (Backward)
}

// -------------------------------------------------------------------------
// WavePose Constructor
// -------------------------------------------------------------------------
WavePose::WavePose() : phase(0.0f) {}

// -------------------------------------------------------------------------
// WavePose::reset
// Resets the wave animation to the beginning.
// -------------------------------------------------------------------------
void WavePose::reset() {
    phase = 0.0f;
}

// -------------------------------------------------------------------------
// WavePose::calculate
// Computes the servo angles for sitting and waving a paw.
// -------------------------------------------------------------------------
void WavePose::calculate(float dt, const JoystickData& inputs, int servoAngles[4]) {
    // -------------------------------------------------------------------------
    // The WavePose makes the robot sit, lift its front-right leg, and oscillate it
    // up and down (using a sine wave) to simulate waving.
    // -------------------------------------------------------------------------
    int pitchOffset = inputs.pitch * 0.2f;
    int rollOffset = inputs.roll * 0.2f;

    // Advance the wave animation phase
    phase += 15.0f * dt; 
    if (phase > 2 * 3.14159f) phase -= 2 * 3.14159f;
    
    // Calculate the waving amplitude (up to 35 degrees deviation)
    int waveAngle = sin(phase) * 35; 
    
    servoAngles[0] = 90 + pitchOffset + rollOffset;                 // Front Left: Planted straight down
    servoAngles[1] = 20 + waveAngle + pitchOffset - rollOffset;     // Front Right: Lifted (base 20) + waving
    servoAngles[2] = 30 - pitchOffset + rollOffset;                 // Hind Left: Folded (sitting)
    servoAngles[3] = 30 - pitchOffset - rollOffset;                 // Hind Right: Folded (sitting)
}

// -------------------------------------------------------------------------
// PeePose Constructor
// -------------------------------------------------------------------------
PeePose::PeePose() : elapsedTime(0.0f) {}

// -------------------------------------------------------------------------
// PeePose::reset
// Resets the timer for the pee animation.
// -------------------------------------------------------------------------
void PeePose::reset() {
    elapsedTime = 0.0f;
}

// -------------------------------------------------------------------------
// PeePose::calculate
// Computes the servo angles for lifting and wagging a hind leg.
// -------------------------------------------------------------------------
void PeePose::calculate(float dt, const JoystickData& inputs, int servoAngles[4]) {
    // -------------------------------------------------------------------------
    // The PeePose simulates a dog lifting its hind leg. The animation runs for 
    // exactly 4 seconds, after which it returns to a normal standing pose.
    // -------------------------------------------------------------------------
    int pitchOffset = inputs.pitch * 0.2f;
    int rollOffset = inputs.roll * 0.2f;

    elapsedTime += dt;

    if (elapsedTime < 4.0f) {
        // Animation is active: plant 3 legs, lift and wag the hind-right leg
        servoAngles[0] = 90 + pitchOffset + rollOffset; // Front Left
        servoAngles[1] = 90 + pitchOffset - rollOffset; // Front Right
        servoAngles[2] = 90 - pitchOffset + rollOffset; // Hind Left
        
        // Calculate a wagging oscillation
        int wag = sin(elapsedTime * 15.0f) * 10;
        
        // Hind right swings backward (130) and oscillates +/- 10 degrees
        servoAngles[3] = 130 + wag - pitchOffset - rollOffset; 
    } else {
        // Animation finished: Return to normal standing position
        servoAngles[0] = 90 + pitchOffset + rollOffset;
        servoAngles[1] = 90 + pitchOffset - rollOffset;
        servoAngles[2] = 90 - pitchOffset + rollOffset;
        servoAngles[3] = 90 - pitchOffset - rollOffset;
    }
}

// -------------------------------------------------------------------------
// ScrapePose Constructor
// -------------------------------------------------------------------------
ScrapePose::ScrapePose() : phase(0.0f) {}

// -------------------------------------------------------------------------
// ScrapePose::calculate
// Computes the servo angles for a bull-like scrape animation.
// -------------------------------------------------------------------------
void ScrapePose::calculate(float dt, const JoystickData& inputs, int servoAngles[4]) {
    // -------------------------------------------------------------------------
    // The ScrapePose simulates an angry bull scraping the ground.
    // Front legs are planted, and the hind legs rapidly alternate swinging back and forth.
    // -------------------------------------------------------------------------
    int pitchOffset = inputs.pitch * 0.2f;
    int rollOffset = inputs.roll * 0.2f;

    phase += 15.0f * dt; // Fast sweeping animation speed
    if (phase > 2 * 3.14159f) phase -= 2 * 3.14159f;

    // Use a sine wave to alternate the legs.
    // When one swings forward, the other swings backward.
    int sweepAngleLeft = sin(phase) * 30; // +/- 30 degrees
    int sweepAngleRight = -sin(phase) * 30; // Opposite phase

    // Plant front legs slightly bent forward (60 degrees) to lower the front
    servoAngles[0] = 60 + pitchOffset + rollOffset;
    servoAngles[1] = 60 + pitchOffset - rollOffset;
    
    // Apply sweeping motion to hind legs
    servoAngles[2] = 90 + sweepAngleLeft - pitchOffset + rollOffset;
    servoAngles[3] = 90 + sweepAngleRight - pitchOffset - rollOffset;
}

// -------------------------------------------------------------------------
// InfoPose::calculate
// Computes the servo angles for sitting while looking up.
// -------------------------------------------------------------------------
void InfoPose::calculate(float dt, const JoystickData& inputs, int servoAngles[4]) {
    // Just sit quietly while showing weather
    int pitchOffset = inputs.pitch * 0.2f;
    int rollOffset = inputs.roll * 0.2f;
    
    servoAngles[0] = 90 + pitchOffset + rollOffset;
    servoAngles[1] = 90 + pitchOffset - rollOffset;
    servoAngles[2] = 30 - pitchOffset + rollOffset;
    servoAngles[3] = 30 - pitchOffset - rollOffset;
}
