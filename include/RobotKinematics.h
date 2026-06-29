#pragma once
#include "Interfaces.h"

/**
 * @class RobotKinematics
 * @brief Core engine that calculates leg movements and coordinates transitions.
 * 
 * This class runs in the main loop (`tick()`) and computes the exact servo 
 * angles based on joystick input and the currently selected Gait or Pose.
 */
class RobotKinematics : public IInputReceiver {
public:
    /**
     * @brief Constructs a new Robot Kinematics engine.
     * @param servoDriver Reference to the hardware PWM driver.
     * @param configRepo Reference to the configuration storage.
     * @param gaitsArray Array of pointers to available gait strategies.
     * @param count Number of gait strategies provided.
     */
    RobotKinematics(IServoDriver& servoDriver, IConfigRepository& configRepo, IGaitStrategy** gaitsArray, int count);

    /**
     * @brief Destroys the Robot Kinematics object.
     */
    ~RobotKinematics() override = default;

    /**
     * @brief Initializes the servo driver and attaches the servos.
     */
    void begin();
    
    // From IInputReceiver
    /**
     * @brief Receives updated joystick values from the input subsystem (e.g., WebUI).
     * @param throttle Forward/backward speed (-100 to 100).
     * @param yaw Left/right turn command (-100 to 100).
     * @param pitch Nose up/down tilt command (-100 to 100).
     * @param roll Left/right tilt command (-100 to 100).
     */
    void onJoystickUpdate(float throttle, float yaw, float pitch, float roll) override;

    /**
     * @brief Notifies the engine that the servo configuration (offsets/inversions) has changed.
     * Triggers a reattachment and homing of the servos.
     */
    void onConfigUpdated() override;
    
    /**
     * @brief Changes the active gait or pose, potentially triggering a physical transition sequence.
     * @param index Index of the gait/pose in the gaits array.
     */
    void setGait(int index) override;

    /**
     * @brief Calculates kinematics for the current frame and writes to the servos. 
     * Must be called continuously in the main loop.
     */
    void tick();
    
    // Expose inputs for other components (like DisplayManager)
    /**
     * @brief Returns a reference to the latest joystick inputs.
     * @return Constant reference to the JoystickData struct.
     */
    const JoystickData& getLatestInputs() const { return latestInputs; }
    
    /**
     * @brief Gets the currently active (or soon to be active) gait index.
     * @return The integer index of the gait.
     */
    int getGaitIndex() const;

private:
    /**
     * @brief Attaches (or re-attaches) servos to their configured pins and homes them to 90 degrees.
     */
    void reattachServos();

    /**
     * @brief Applies configuration logic (offset, inversion, limits) to a target angle.
     * @param servoIndex The index (0-3) of the servo.
     * @param targetAngle The mathematically calculated angle (0-180).
     * @param applyLimits Whether to enforce the configured safety limits.
     * @return The final, safe angle to write to the physical servo.
     */
    int processAngle(int servoIndex, int targetAngle, bool applyLimits = true);

    IServoDriver& driver;
    IConfigRepository& config;
    IGaitStrategy** gaits;
    int numGaits;
    IGaitStrategy* currentGait;
    int currentGaitIndex;
    
    float transitionProgress;
    int pendingGaitIndex;
    
    JoystickData latestInputs;
    unsigned long lastTick;
};
