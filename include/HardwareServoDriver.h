#pragma once
#include "Interfaces.h"
#include <ESP32Servo.h>

class HardwareServoDriver : public IServoDriver {
public:
    HardwareServoDriver();
    ~HardwareServoDriver() override = default;

    void begin() override;
    void attach(int index, int pin) override;
    void detach(int index) override;
    bool isAttached(int index) override;
    void write(int index, int angle) override;

private:
    Servo servos[4];
};
