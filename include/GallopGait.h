#pragma once
#include "Interfaces.h"

class GallopGait : public IGaitStrategy {
public:
    GallopGait();
    void calculate(float dt, const JoystickData& inputs, int servoAngles[4]) override;

private:
    float phase;
};
