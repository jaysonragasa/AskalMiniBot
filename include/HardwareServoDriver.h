#pragma once
#include "Interfaces.h"
#include <ESP32Servo.h>

/**
 * @class HardwareServoDriver
 * @brief Implementation of IServoDriver using ESP32Servo library.
 * 
 * Maps logical servo indices (0-3) to physical PWM pins and handles the hardware 
 * timers required for the ESP32.
 */
class HardwareServoDriver : public IServoDriver {
public:
    /**
     * @brief Constructs a new Hardware Servo Driver.
     */
    HardwareServoDriver();

    /**
     * @brief Destroys the Hardware Servo Driver.
     */
    ~HardwareServoDriver() override = default;

    /**
     * @brief Allocates ESP32 PWM timers for the servos.
     */
    void begin() override;
    
    /**
     * @brief Attaches a servo to a specific GPIO pin.
     * @param index Servo logical index (0-3).
     * @param pin Physical GPIO pin number.
     */
    void attach(int index, int pin) override;

    /**
     * @brief Detaches a servo from its GPIO pin.
     * @param index Servo logical index (0-3).
     */
    void detach(int index) override;

    /**
     * @brief Checks if a servo is currently attached.
     * @param index Servo logical index (0-3).
     * @return true if attached, false otherwise.
     */
    bool isAttached(int index) override;

    /**
     * @brief Commands the servo to a specific angle.
     * @param index Servo logical index (0-3).
     * @param angle Target angle in degrees (usually 0-180).
     */
    void write(int index, int angle) override;

private:
    Servo servos[4]; ///< Hardware servo objects.
};
