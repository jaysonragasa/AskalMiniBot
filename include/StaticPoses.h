#pragma once
#include "Interfaces.h"

/**
 * @class SitPose
 * @brief Stationary pose where the robot rests on its folded hind legs.
 */
class SitPose : public IGaitStrategy {
public:
    /**
     * @brief Calculates the servo angles for the Sit pose.
     * @param dt Delta time since the last frame (unused here).
     * @param inputs Current joystick inputs for dynamically tilting the pose.
     * @param servoAngles Output array for calculated angles.
     */
    void calculate(float dt, const JoystickData& inputs, int servoAngles[4]) override;

    /**
     * @brief Returns whether mechanical safety limits should be applied.
     * @return false (limits are ignored to allow extreme folding).
     */
    bool applyLimits() const override { return false; }

    /**
     * @brief Indicates if this pose has legs folded under the robot.
     * @return true, as hind legs are folded.
     */
    bool isFoldedPose() const override { return true; }
};

/**
 * @class StretchPose
 * @brief Pose used both for stretching and for physical transitions.
 * Front legs point forward, hind legs point backward.
 */
class StretchPose : public IGaitStrategy {
public:
    /**
     * @brief Calculates the servo angles for the Stretch pose.
     * @param dt Delta time since the last frame (unused here).
     * @param inputs Current joystick inputs for dynamically tilting the pose.
     * @param servoAngles Output array for calculated angles.
     */
    void calculate(float dt, const JoystickData& inputs, int servoAngles[4]) override;

    /**
     * @brief Returns whether mechanical safety limits should be applied.
     * @return false (limits are ignored to allow full stretching).
     */
    bool applyLimits() const override { return false; }
};

/**
 * @class WavePose
 * @brief Robot sits and lifts one front leg to wave up and down.
 */
class WavePose : public IGaitStrategy {
private:
    float phase; ///< Animation phase for the waving arm.
public:
    /**
     * @brief Constructs a new Wave Pose object and initializes phase.
     */
    WavePose();

    /**
     * @brief Calculates the servo angles for the animated Wave pose.
     * @param dt Delta time to progress the animation.
     * @param inputs Current joystick inputs.
     * @param servoAngles Output array for calculated angles.
     */
    void calculate(float dt, const JoystickData& inputs, int servoAngles[4]) override;

    /**
     * @brief Returns whether mechanical safety limits should be applied.
     * @return false (limits are ignored).
     */
    bool applyLimits() const override { return false; }

    /**
     * @brief Resets the wave animation phase to 0.
     */
    void reset() override;

    /**
     * @brief Indicates if this pose has legs folded under the robot.
     * @return true, as hind legs are folded in a sit-like state.
     */
    bool isFoldedPose() const override { return true; }
};

/**
 * @class PeePose
 * @brief Playful pose where the robot lifts its hind leg and wags it.
 */
class PeePose : public IGaitStrategy {
private:
    float elapsedTime; ///< Tracks how long the pee animation has been running.
public:
    /**
     * @brief Constructs a new Pee Pose object and initializes the elapsed time.
     */
    PeePose();

    /**
     * @brief Calculates the servo angles for the Pee animation.
     * @param dt Delta time to progress the animation.
     * @param inputs Current joystick inputs.
     * @param servoAngles Output array for calculated angles.
     */
    void calculate(float dt, const JoystickData& inputs, int servoAngles[4]) override;

    /**
     * @brief Returns whether mechanical safety limits should be applied.
     * @return false.
     */
    bool applyLimits() const override { return false; }

    /**
     * @brief Resets the elapsed time for the animation.
     */
    void reset() override;
};

/**
 * @class ScrapePose
 * @brief Angry bull pose: scrapes hind legs alternatingly on the ground.
 */
class ScrapePose : public IGaitStrategy {
private:
    float phase; ///< Animation phase for the alternating scrape.
public:
    /**
     * @brief Constructs a new Scrape Pose object and initializes phase.
     */
    ScrapePose();

    /**
     * @brief Calculates the servo angles for the Scrape animation.
     * @param dt Delta time to progress the animation.
     * @param inputs Current joystick inputs.
     * @param servoAngles Output array for calculated angles.
     */
    void calculate(float dt, const JoystickData& inputs, int servoAngles[4]) override;

    /**
     * @brief Returns whether mechanical safety limits should be applied.
     * @return false.
     */
    bool applyLimits() const override { return false; }
};

/**
 * @class InfoPose
 * @brief Stationary sitting pose used when displaying weather info on the OLED.
 */
class InfoPose : public IGaitStrategy {
public:
    /**
     * @brief Calculates the servo angles for the Info pose.
     * @param dt Delta time since the last frame (unused here).
     * @param inputs Current joystick inputs.
     * @param servoAngles Output array for calculated angles.
     */
    void calculate(float dt, const JoystickData& inputs, int servoAngles[4]) override;

    /**
     * @brief Returns whether mechanical safety limits should be applied.
     * @return false.
     */
    bool applyLimits() const override { return false; }

    /**
     * @brief Indicates if this pose has legs folded under the robot.
     * @return true, as it is a sitting variation.
     */
    bool isFoldedPose() const override { return true; }
};
