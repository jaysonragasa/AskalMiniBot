#pragma once
#include "Interfaces.h"

class TrotGait : public IGaitStrategy {
public:
    TrotGait();
    ~TrotGait() override = default;

    void calculate(float dt, const JoystickData& inputs, int servoAngles[4]) override;

private:
    float phase;
};
