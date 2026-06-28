#pragma once
#include "Interfaces.h"

class WalkGait : public IGaitStrategy {
public:
    WalkGait();
    void calculate(float dt, const JoystickData& inputs, int servoAngles[4]) override;

private:
    float phase;
};
