#pragma once
#include <stdint.h>

struct ServoConfig {
    int pin;
    float maxAngle;
    int8_t invert;  // 1 for normal, -1 for inverted
    int8_t enabled; // 1 for enabled, 0 for disabled
    float offset;
};

// Interface for saving and loading configurations
class IConfigRepository {
public:
    virtual ~IConfigRepository() = default;
    virtual void begin() = 0;
    virtual ServoConfig getServoConfig(int index) = 0;
    virtual void setServoConfig(int index, const ServoConfig& config) = 0;
};

// Interface for controlling physical servos
class IServoDriver {
public:
    virtual ~IServoDriver() = default;
    virtual void begin() = 0;
    virtual void attach(int index, int pin) = 0;
    virtual void detach(int index) = 0;
    virtual bool isAttached(int index) = 0;
    virtual void write(int index, int angle) = 0;
};

// Struct to hold joystick input data
struct JoystickData {
    float throttle;
    float yaw;
    float pitch;
    float roll;
};

// Interface for calculating gait angles
class IGaitStrategy {
public:
    virtual ~IGaitStrategy() = default;
    virtual void calculate(float dt, const JoystickData& inputs, int servoAngles[4]) = 0;
    virtual bool applyLimits() const { return true; }
};

// Interface for receiving joystick inputs (implemented by Kinematics)
class IInputReceiver {
public:
    virtual ~IInputReceiver() = default;
    virtual void onJoystickUpdate(float throttle, float yaw, float pitch, float roll) = 0;
    virtual void onConfigUpdated() = 0;
    virtual void setGait(int index) = 0;
};
