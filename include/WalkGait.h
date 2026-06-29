#pragma once
#include "Interfaces.h"

/**
 * @class WalkGait
 * @brief Implements a slow, stable 4-beat walking motion.
 * 
 * In a walk, each leg moves at a different phase (e.g., separated by PI/2).
 * This ensures that at least 3 legs are typically supporting the robot at any time,
 * maximizing stability at the cost of speed.
 */
class WalkGait : public IGaitStrategy {
public:
    /**
     * @brief Constructs a new Walk Gait object and initializes phase.
     */
    WalkGait();

    /**
     * @brief Calculates the target servo angles for the next frame of the walk animation.
     * @param dt Delta time in seconds since the last frame.
     * @param inputs Current joystick inputs for steering and speed.
     * @param servoAngles Array of 4 integers where the resulting servo angles will be written.
     */
    void calculate(float dt, const JoystickData& inputs, int servoAngles[4]) override;

private:
    float phase; ///< The current progress of the walk cycle (0 to 2*PI).
};
