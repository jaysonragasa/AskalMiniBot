#pragma once
#include "Interfaces.h"

/**
 * @class TrotGait
 * @brief Implements a moderate-speed trotting motion for the robot.
 * 
 * In a trot, diagonal pairs of legs move together in sync (Front Left + Hind Right, 
 * and Front Right + Hind Left). This is faster than a walk but less stable.
 */
class TrotGait : public IGaitStrategy {
public:
    /**
     * @brief Constructs a new Trot Gait object and initializes phase.
     */
    TrotGait();

    /**
     * @brief Destroys the Trot Gait object.
     */
    ~TrotGait() override = default;

    /**
     * @brief Calculates the target servo angles for the next frame of the trot animation.
     * @param dt Delta time in seconds since the last frame.
     * @param inputs Current joystick inputs for steering and speed.
     * @param servoAngles Array of 4 integers where the resulting servo angles will be written.
     */
    void calculate(float dt, const JoystickData& inputs, int servoAngles[4]) override;

private:
    float phase; ///< The current progress of the trot cycle (0 to 2*PI).
};
