#pragma once
#include "Interfaces.h"

class RobotKinematics : public IInputReceiver {
public:
    RobotKinematics(IServoDriver& servoDriver, IConfigRepository& configRepo, IGaitStrategy** gaitsArray, int count);
    ~RobotKinematics() override = default;

    void begin();
    
    // From IInputReceiver
    void onJoystickUpdate(float throttle, float yaw, float pitch, float roll) override;
    void onConfigUpdated() override;
    void setGait(int index) override;

    // Call this in the main loop
    void tick();
    
    // Expose inputs for other components (like DisplayManager)
    const JoystickData& getLatestInputs() const { return latestInputs; }
    
    int getGaitIndex() const;

private:
    void reattachServos();
    int processAngle(int servoIndex, int targetAngle, bool applyLimits = true);

    IServoDriver& driver;
    IConfigRepository& config;
    IGaitStrategy** gaits;
    int numGaits;
    IGaitStrategy* currentGait;
    int currentGaitIndex;
    
    JoystickData latestInputs;
    unsigned long lastTick;
};
