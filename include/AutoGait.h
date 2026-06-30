#pragma once
#include "Interfaces.h"

/**
 * @class AutoGait
 * @brief Parametrically blends between a Walk (low speed) and Gallop (high speed).
 */
class AutoGait : public IGaitStrategy {
private:
    float phase; ///< Unified animation phase for the gait

public:
    /**
     * @brief Constructs a new AutoGait.
     */
    AutoGait();

    /**
     * @brief Calculates servo angles by blending Walk and Gallop parameters based on speed.
     * @param dt Delta time since the last frame.
     * @param inputs Current joystick inputs.
     * @param servoAngles Output array for calculated angles.
     */
    void calculate(float dt, const JoystickData& inputs, int servoAngles[4]) override;

    /**
     * @brief Resets the animation phase.
     */
    void reset() override;

    /**
     * @brief Returns whether mechanical safety limits should be applied.
     */
    bool applyLimits() const override { return true; }
};
