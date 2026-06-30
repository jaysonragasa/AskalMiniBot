#pragma once
#include <stdint.h>

/**
 * @struct ServoConfig
 * @brief Per-servo calibration and runtime configuration.
 *
 * One instance exists for each of the 4 legs. Values are loaded from / saved to
 * non-volatile storage by an IConfigRepository and consumed by RobotKinematics
 * when mapping a logical target angle to a physical servo command.
 */
struct ServoConfig {
    int pin;        ///< Physical GPIO pin the servo signal wire is attached to.
    float maxAngle; ///< Max allowed deviation (in degrees) from the 90 center when limits are applied.
    int8_t invert;  ///< Direction multiplier: 1 for normal, -1 for inverted.
    int8_t enabled; ///< 1 if the servo is driven, 0 to hold it at mechanical center (90).
    float offset;   ///< Manual trim (in degrees) added after centering to calibrate the neutral pose.
};

#include <Arduino.h>

/**
 * @class IConfigRepository
 * @brief Abstraction for persisting and retrieving robot configuration.
 *
 * Decouples the rest of the system from the storage backend (e.g. ESP32 NVS).
 * Holds both per-servo calibration and the OpenWeather integration settings.
 */
class IConfigRepository {
public:
    /**
     * @brief Virtual destructor for safe polymorphic deletion.
     */
    virtual ~IConfigRepository() = default;

    /**
     * @brief Initializes the backing store and loads settings into memory.
     */
    virtual void begin() = 0;

    /**
     * @brief Retrieves the configuration for a specific servo.
     * @param index Servo index (0-3: Front-Left, Front-Right, Hind-Left, Hind-Right).
     * @return The ServoConfig for that servo.
     */
    virtual ServoConfig getServoConfig(int index) = 0;

    /**
     * @brief Persists the configuration for a specific servo.
     * @param index Servo index (0-3).
     * @param config The configuration to store.
     */
    virtual void setServoConfig(int index, const ServoConfig& config) = 0;
    
    // OpenWeather settings
    /**
     * @brief Retrieves the saved OpenWeather API key.
     * @return The API key, or an empty string if unset.
     */
    virtual String getOpenWeatherKey() = 0;

    /**
     * @brief Persists the OpenWeather API key.
     * @param key The API key to store.
     */
    virtual void setOpenWeatherKey(const String& key) = 0;

    /**
     * @brief Retrieves the saved latitude used for weather queries.
     * @return Latitude in decimal degrees.
     */
    virtual float getLatitude() = 0;

    /**
     * @brief Persists the latitude used for weather queries.
     * @param lat Latitude in decimal degrees.
     */
    virtual void setLatitude(float lat) = 0;

    /**
     * @brief Retrieves the saved longitude used for weather queries.
     * @return Longitude in decimal degrees.
     */
    virtual float getLongitude() = 0;

    /**
     * @brief Persists the longitude used for weather queries.
     * @param lon Longitude in decimal degrees.
     */
    virtual void setLongitude(float lon) = 0;
};

/**
 * @class IServoDriver
 * @brief Abstraction for the physical servo hardware layer.
 *
 * Maps logical servo indices (0-3) to PWM outputs, hiding the underlying
 * driver library (e.g. ESP32Servo) from the kinematics engine.
 */
class IServoDriver {
public:
    /**
     * @brief Virtual destructor for safe polymorphic deletion.
     */
    virtual ~IServoDriver() = default;

    /**
     * @brief Initializes the driver (e.g. allocates PWM timers).
     */
    virtual void begin() = 0;

    /**
     * @brief Attaches a servo to a physical pin so it can be driven.
     * @param index Servo logical index (0-3).
     * @param pin Physical GPIO pin number.
     */
    virtual void attach(int index, int pin) = 0;

    /**
     * @brief Detaches a servo, releasing its pin and stopping its signal.
     * @param index Servo logical index (0-3).
     */
    virtual void detach(int index) = 0;

    /**
     * @brief Reports whether a servo is currently attached.
     * @param index Servo logical index (0-3).
     * @return true if attached, false otherwise.
     */
    virtual bool isAttached(int index) = 0;

    /**
     * @brief Commands a servo to a target angle.
     * @param index Servo logical index (0-3).
     * @param angle Target angle in degrees (typically 0-180).
     */
    virtual void write(int index, int angle) = 0;
};

/**
 * @struct JoystickData
 * @brief Snapshot of the operator's control inputs.
 *
 * Each axis ranges from -100 to 100. The left stick drives throttle/yaw and the
 * right stick drives pitch/roll (see the Web UI). Consumed by gait strategies
 * for steering and by RobotKinematics for stationary tilt overrides.
 */
struct JoystickData {
    float throttle; ///< Forward/backward speed command (-100 to 100).
    float yaw;      ///< Left/right turn command (-100 to 100).
    float pitch;    ///< Nose up/down tilt command (-100 to 100).
    float roll;     ///< Left/right tilt command (-100 to 100).
};

/**
 * @class IGaitStrategy
 * @brief Strategy interface for a single gait or pose.
 *
 * Each concrete strategy turns the current joystick inputs into four target
 * servo angles. Cyclic gaits advance an internal phase; static/animated poses
 * may use a timer or none at all. Selected and driven by RobotKinematics.
 */
class IGaitStrategy {
public:
    /**
     * @brief Virtual destructor for safe polymorphic deletion.
     */
    virtual ~IGaitStrategy() = default;

    /**
     * @brief Computes the target servo angles for the next frame.
     * @param dt Delta time in seconds since the last frame.
     * @param inputs Current joystick inputs.
     * @param servoAngles Output array of 4 angles (one per leg, indices 0-3).
     */
    virtual void calculate(float dt, const JoystickData& inputs, int servoAngles[4]) = 0;

    /**
     * @brief Whether RobotKinematics should clamp this strategy's output to the per-servo safety limits.
     * @return true to enforce maxAngle limits (default); false for poses that need extreme folding.
     */
    virtual bool applyLimits() const { return true; }

    /**
     * @brief Hook called when this strategy becomes the active gait. Use it to reset phase/timers.
     */
    virtual void reset() {} // Called when the gait is newly selected

    /**
     * @brief Whether this pose folds the hind legs under the body (sit-like).
     * @return true if folded; used to trigger a safe stretch transition before standing.
     */
    virtual bool isFoldedPose() const { return false; } // Returns true if the pose folds the hind legs under the body
};

/**
 * @class IInputReceiver
 * @brief Abstraction for the object that consumes control and configuration events.
 *
 * Implemented by RobotKinematics and called by the WebUIManager so the UI layer
 * has no direct dependency on the kinematics engine.
 */
class IInputReceiver {
public:
    /**
     * @brief Virtual destructor for safe polymorphic deletion.
     */
    virtual ~IInputReceiver() = default;

    /**
     * @brief Delivers an updated set of joystick values.
     * @param throttle Forward/backward speed (-100 to 100).
     * @param yaw Left/right turn (-100 to 100).
     * @param pitch Nose up/down tilt (-100 to 100).
     * @param roll Left/right tilt (-100 to 100).
     */
    virtual void onJoystickUpdate(float throttle, float yaw, float pitch, float roll) = 0;

    /**
     * @brief Notifies the receiver that the servo configuration has changed.
     */
    virtual void onConfigUpdated() = 0;

    /**
     * @brief Requests a change of the active gait/pose.
     * @param index Index of the gait/pose to activate.
     */
    virtual void setGait(int index) = 0;
};
