# AskalMiniBot

A 1-DOF-per-leg quadruped robot dog firmware for the ESP32-S3 (DFRobot FireBeetle 2 ESP32-S3, FH4R2 / 4MB flash / 2MB PSRAM). Each of the 4 legs is driven by a single servo, so all motion (gaits and poses) is produced by swinging four legs about a logical 90° center. The robot is controlled from a browser over WiFi via a virtual dual-joystick Web UI, shows procedural eyes and OpenWeather data on an optional OLED, and supports OTA firmware updates.

## Build & Flash

This is a PlatformIO project (not Arduino IDE). Use PlatformIO, not `arduino-cli`.

- Build: `pio run`
- Upload (USB): `pio run -t upload`
- Monitor: `pio device monitor` (baud 115200)
- OTA: uncomment `upload_protocol = espota` and `upload_port` in `platformio.ini` (hostname `AskalMiniBot`)

Do not start the monitor or a long-running flash as a blocking command; suggest the user run it, or run builds with a single invocation.

## Architecture (SOLID)

The codebase is deliberately structured around SOLID principles with dependency injection wired up in `main.cpp`. Preserve this structure when making changes.

- `include/Interfaces.h` is the contract hub. It defines the abstractions everything depends on:
  - `IConfigRepository` — persistence of servo calibration and weather settings
  - `IServoDriver` — physical servo control
  - `IGaitStrategy` — a single gait or pose (Strategy pattern)
  - `IInputReceiver` — receives joystick/config/gait commands (implemented by kinematics)
  - Data structs: `ServoConfig`, `JoystickData`
- Concrete implementations depend only on these interfaces, never on each other directly. Constructor injection is used throughout (see `RobotKinematics`, `WebUIManager`, `DisplayManager`).
- `main.cpp` is the composition root: it instantiates every concrete class, builds the `allGaits[]` array, and injects dependencies. New gaits/poses are registered here.

### Key components

- `RobotKinematics` — core engine. `tick()` runs every loop, asks the active `IGaitStrategy` for target angles, applies pitch/roll overrides, then runs every angle through `processAngle()` (offset, inversion, safety limits) before writing to the driver. Owns the gait-switching state machine.
- `HardwareServoDriver` — `IServoDriver` via the ESP32Servo library; maps logical indices 0-3 to GPIO pins.
- `PreferencesConfig` — `IConfigRepository` backed by ESP32 NVS (`Preferences`); persists across reboots, caches configs in memory.
- `WebUIManager` — `ESPAsyncWebServer` + WebSocket. Serves the single-page UI (HTML/JS in a `PROGMEM` raw string literal) and routes JSON messages to the injected `IInputReceiver`/`IConfigRepository`.
- `DisplayManager` — optional OLED. Runs its own FreeRTOS task pinned to Core 0 so rendering and blocking weather HTTP fetches never stall the kinematics loop on Core 1. Gated by `#define ENABLE_OLED_DISPLAY` in `config.h`.

### Gaits & poses (`IGaitStrategy`)

Cyclic gaits (`TrotGait`, `WalkGait`, `GallopGait`, `AutoGait`) advance a `phase` and use sine waves; static/animated poses live in `StaticPoses.{h,cpp}` (`SitPose`, `StretchPose`, `WavePose`, `PeePose`, `ScrapePose`, `InfoPose`). A strategy overrides:
- `calculate(dt, inputs, servoAngles[4])` — write target angles (required)
- `applyLimits()` — return `false` for poses that need extreme folding beyond `maxAngle`
- `reset()` — called when the gait becomes active (reset phase/timers)
- `isFoldedPose()` — `true` for sit-like poses with hind legs tucked under the body

## Conventions

- Servo index order is fixed everywhere: `0` Front-Left, `1` Front-Right, `2` Hind-Left, `3` Hind-Right.
- `90` is the logical center for every servo. Gaits compute deviations from 90; `processAngle()` then applies per-servo `offset`, `invert` (±1), and `maxAngle` safety clamp.
- The main loop targets a fixed cadence (`LOOP_DELAY_MS = 20`, ~50Hz). Keep `tick()` and `loop()` non-blocking; never add long `delay()` calls in the hot path. Note the deliberate exception: `reattachServos()` staggers servo attach with `delay(300)` to avoid brownout, and `onConfigUpdated()` intentionally skips reattach to keep calibration sliders responsive.
- Heavy or blocking work (HTTP, rendering) belongs on the Core 0 display task, not the main loop.

### Code style

- Heavily commented. Headers use Doxygen (`@brief`, `@param`, `@return`); `.cpp` files use banner comment blocks (`// ----`) to delimit logical steps. Match this style in new code.
- New gait/pose: create `include/<Name>.h` + `src/<Name>.cpp` implementing `IGaitStrategy`, then register the instance in `main.cpp`'s `allGaits[]` and add a matching `mode-btn` (with the correct index) in the Web UI HTML in `WebUIManager.cpp`. Gait indices in `setMode(idx)` must stay aligned with the array order.

### Web UI / WebSocket protocol

Browser → firmware JSON messages, keyed by `type`:
- `joystick` — `{t, y, p, r}` throttle/yaw/pitch/roll (left stick = throttle+yaw, right stick = pitch+roll)
- `set_mode` — `{mode: index}` into `allGaits[]`
- `get_config` / `set_config` — servo pin/offset/invert
- `set_weather_cfg` — OpenWeather key, lat, lon

## Hardware notes

- Servos default to GPIO 1, 2, 13, 14 (`DEFAULT_SERVO_CONFIGS` in `config.h`).
- OLED is I2C: SDA 6, SCL 7, addr 0x3C, 128x64.
- `ARDUINO_USB_CDC_ON_BOOT=1` routes `Serial` to native USB; PSRAM is enabled.

## Caution

- WiFi credentials and any API keys live in `config.h` (committed). Do not echo secret values back in responses; flag before committing changes that add real credentials.
- `.pio/` (libdeps, build) is generated — never edit library sources there; treat them as read-only dependencies.
