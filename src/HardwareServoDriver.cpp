#include "HardwareServoDriver.h"

HardwareServoDriver::HardwareServoDriver() {}

void HardwareServoDriver::begin() {
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);
}

void HardwareServoDriver::attach(int index, int pin) {
    if (index >= 0 && index < 4) {
        servos[index].setPeriodHertz(50);
        servos[index].attach(pin, 500, 2500);
    }
}

void HardwareServoDriver::detach(int index) {
    if (index >= 0 && index < 4 && servos[index].attached()) {
        servos[index].detach();
    }
}

bool HardwareServoDriver::isAttached(int index) {
    if (index >= 0 && index < 4) {
        return servos[index].attached();
    }
    return false;
}

void HardwareServoDriver::write(int index, int angle) {
    if (index >= 0 && index < 4) {
        servos[index].write(angle);
    }
}
