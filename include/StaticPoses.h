#pragma once
#include "Interfaces.h"

class SitPose : public IGaitStrategy {
public:
    void calculate(float dt, const JoystickData& inputs, int servoAngles[4]) override;
    bool applyLimits() const override { return false; }
};

class StretchPose : public IGaitStrategy {
public:
    void calculate(float dt, const JoystickData& inputs, int servoAngles[4]) override;
    bool applyLimits() const override { return false; }
};

class WavePose : public IGaitStrategy {
private:
    float phase;
public:
    WavePose();
    void calculate(float dt, const JoystickData& inputs, int servoAngles[4]) override;
    bool applyLimits() const override { return false; }
    void reset() override;
};

class PeePose : public IGaitStrategy {
private:
    float elapsedTime;
public:
    PeePose();
    void calculate(float dt, const JoystickData& inputs, int servoAngles[4]) override;
    bool applyLimits() const override { return false; }
    void reset() override;
};

class ScrapePose : public IGaitStrategy {
private:
    float phase;
public:
    ScrapePose();
    void calculate(float dt, const JoystickData& inputs, int servoAngles[4]) override;
    bool applyLimits() const override { return false; }
};

class InfoPose : public IGaitStrategy {
public:
    void calculate(float dt, const JoystickData& inputs, int servoAngles[4]) override;
    bool applyLimits() const override { return false; }
};
