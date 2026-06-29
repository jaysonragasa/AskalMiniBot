#include "HardwareServoDriver.h"

// -------------------------------------------------------------------------
// HardwareServoDriver Constructor
// -------------------------------------------------------------------------
HardwareServoDriver::HardwareServoDriver() {}

// -------------------------------------------------------------------------
// HardwareServoDriver::begin
// Allocates PWM timers needed for servo control.
// -------------------------------------------------------------------------
void HardwareServoDriver::begin() {
    // -------------------------------------------------------------------------
    // ESP32 requires explicit PWM timer allocation for servos.
    // We need 4 timers for our 4 legs.
    // -------------------------------------------------------------------------
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);
}

// -------------------------------------------------------------------------
// HardwareServoDriver::attach
// Attaches a servo to a specific GPIO pin.
// -------------------------------------------------------------------------
void HardwareServoDriver::attach(int index, int pin) {
    if (index >= 0 && index < 4) {
        // Standard analog servos operate at 50Hz (20ms period).
        servos[index].setPeriodHertz(50);
        // Attach to pin with standard pulse widths: 500us (0deg) to 2500us (180deg)
        servos[index].attach(pin, 500, 2500);
    }
}

// -------------------------------------------------------------------------
// HardwareServoDriver::detach
// Detaches a servo from its GPIO pin.
// -------------------------------------------------------------------------
void HardwareServoDriver::detach(int index) {
    if (index >= 0 && index < 4 && servos[index].attached()) {
        servos[index].detach();
    }
}

// -------------------------------------------------------------------------
// HardwareServoDriver::isAttached
// Checks if a servo is currently attached.
// -------------------------------------------------------------------------
bool HardwareServoDriver::isAttached(int index) {
    if (index >= 0 && index < 4) {
        return servos[index].attached();
    }
    return false;
}

// -------------------------------------------------------------------------
// HardwareServoDriver::write
// Commands a servo to a specific angle.
// -------------------------------------------------------------------------
void HardwareServoDriver::write(int index, int angle) {
    if (index >= 0 && index < 4) {
        servos[index].write(angle);
    }
}
