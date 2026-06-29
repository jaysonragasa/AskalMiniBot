#pragma once
#include "Interfaces.h"

/**
 * @class GallopGait
 * @brief Implements a high-speed bounding/galloping motion for the robot.
 * 
 * Unlike walking or trotting, a gallop moves both front legs together, 
 * followed by both hind legs together, creating a bounding effect.
 */
class GallopGait : public IGaitStrategy {
public:
    /**
     * @brief Constructs a new Gallop Gait object and initializes phase.
     */
    GallopGait();

    /**
     * @brief Calculates the target servo angles for the next frame of the gallop animation.
     * @param dt Delta time in seconds since the last frame.
     * @param inputs Current joystick inputs for steering and speed.
     * @param servoAngles Array of 4 integers where the resulting servo angles will be written.
     */
    void calculate(float dt, const JoystickData& inputs, int servoAngles[4]) override;

private:
    float phase; ///< The current progress of the gallop cycle (0 to 2*PI).
};
