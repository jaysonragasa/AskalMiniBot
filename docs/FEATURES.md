# AskalMiniBot — Feature & Architecture Guide

A developer-facing overview of what the firmware does and how the pieces fit
together. For build/flash commands and coding conventions see the steering
files in `.kiro/steering/`. For API-level detail, read the Doxygen comments in
the headers under `include/`.

---

## 1. What it is

AskalMiniBot is a **1-DOF-per-leg quadruped robot dog** running on an
**ESP32-S3** (DFRobot FireBeetle 2 ESP32-S3, FH4R2: 4MB flash / 2MB PSRAM).

Each of the four legs is driven by a **single servo**, so every motion — walking,
trotting, sitting, waving — is produced by swinging four legs about a logical
**90° center**. There is no knee/hip articulation; expression comes from gait
timing and amplitude, not extra joints.

The robot is controlled from a phone/browser over WiFi using a virtual
dual-joystick web page, shows procedural "eyes" and live weather on an optional
OLED, and supports OTA firmware updates.

---

## 2. Hardware

| Item | Detail |
|------|--------|
| MCU | ESP32-S3 (FireBeetle 2 ESP32-S3 target) |
| Flash / PSRAM | 4MB QIO flash, 2MB QSPI PSRAM |
| Servos | 4× standard analog servos, 50Hz, 500–2500µs pulse |
| Servo pins (default) | FL=GPIO1, FR=GPIO2, HL=GPIO13, HR=GPIO14 |
| OLED | SSD1306 128×64 over I2C — SDA=GPIO6, SCL=GPIO7, addr 0x3C |
| Serial | 115200 baud over native USB (`ARDUINO_USB_CDC_ON_BOOT=1`) |

Servo defaults live in `DEFAULT_SERVO_CONFIGS` in `include/config.h`. Right-side
servos (FR, HR) default to inverted because they are physically mirrored.

### Leg index convention (fixed everywhere)

| Index | Leg |
|-------|-----|
| 0 | Front-Left (FL) |
| 1 | Front-Right (FR) |
| 2 | Hind-Left (HL) |
| 3 | Hind-Right (HR) |

---

## 3. Software architecture (SOLID)

Dependencies are defined as interfaces in `include/Interfaces.h` and wired
together in `src/main.cpp` (the composition root) via constructor injection.

```
                         +------------------+
   Browser  --WebSocket--> WebUIManager     |
                         +---------+--------+
                                   | IInputReceiver / IConfigRepository
                                   v
   +-------------------+     +------------------+     +---------------------+
   | IConfigRepository |<----+ RobotKinematics  +---->|   IServoDriver      |
   | PreferencesConfig |     | (core engine)    |     | HardwareServoDriver |
   |   (NVS storage)   |     +--------+---------+     +---------------------+
   +-------------------+              | IGaitStrategy*[]
                                      v
            Trot / Walk / Gallop / Auto / Sit / Stretch / Wave / Pee / Scrape / Info

   DisplayManager (optional) runs on its own FreeRTOS task (Core 0).
```

| Component | Interface | Responsibility |
|-----------|-----------|----------------|
| `RobotKinematics` | `IInputReceiver` | Core engine. Per-tick: ask active gait for angles, apply pitch/roll overrides, clamp & calibrate, write to servos. Owns gait switching + the stretch transition. |
| `HardwareServoDriver` | `IServoDriver` | ESP32Servo PWM; maps index 0-3 → GPIO. |
| `PreferencesConfig` | `IConfigRepository` | NVS-backed calibration + weather settings. |
| `WebUIManager` | — | Serves the embedded web page; routes WebSocket JSON. |
| `DisplayManager` | — | Optional OLED on Core 0: eyes, boot splash, weather. |
| `*Gait` / `*Pose` | `IGaitStrategy` | One movement each (see §5). |

The dual-core split matters: kinematics/WiFi run on Core 1; the OLED task is
pinned to Core 0 so its blocking I2C writes and HTTP weather fetches never
introduce jitter into the servo PWM.

---

## 4. Gait & pose registry (IMPORTANT)

The order of `allGaits[]` in `main.cpp` **defines the mode index** used by the
Web UI buttons (`set_mode`), `RobotKinematics::setGait()`, and
`DisplayManager`. This table is the single source of truth — keep it in sync
with `main.cpp` and the Web UI buttons in `WebUIManager.cpp`.

| Index | Name | Type | Folded? | Limits | Notes |
|-------|------|------|---------|--------|-------|
| 0 | Trot | Cyclic gait | no | yes | Diagonal pairs, 180° apart. Phase mult 10. |
| 1 | Walk | Cyclic gait | no | yes | Stable 4-beat, legs at PI/2 offsets. Phase mult 6. |
| 2 | Gallop | Cyclic gait | no | yes | Bounding; hind pair offset 3·PI/4. Phase mult 15. |
| 3 | Auto | Cyclic gait | no | yes | Blends Walk→Gallop by speed (see §6). |
| 4 | Sit | Static pose | yes | no | Hind legs folded under (30°). |
| 5 | Stretch | Static pose | no | no | Front fwd (30°), hind back (150°). Also the transition pose. |
| 6 | Wave | Animated pose | yes | no | Sits, lifts FR leg, oscillates to "wave". |
| 7 | Pee | Animated pose | no | no | Lifts/wags HR leg for ~4s, then stands. |
| 8 | Scrape | Animated pose | no | no | "Angry bull" — hind legs scrape alternately. |
| 9 | Info | Static pose | yes | no | Sits quietly; triggers the OLED weather screen. |

**Folded?** = `isFoldedPose()`. When switching FROM a folded pose TO a
non-folded one, `RobotKinematics` runs a smooth stretch transition first so the
robot doesn't flip backward.

**Limits** = `applyLimits()`. Poses return `false` so legs can fold/extend past
the normal `maxAngle` safety clamp used by walking gaits.

### OLED reaction indices (DisplayManager)

`DisplayManager.cpp` keys its OLED behavior off the gait index. These are now
aligned with the `allGaits[]` order above:

| DisplayManager check | Pose | Behavior |
|----------------------|------|----------|
| `currentGaitIndex == 7` | Pee | HAPPY eyes |
| `currentGaitIndex == 8` | Scrape | ANGRY eyes |
| `currentGaitIndex == 9` | Info | Weather screen |

> Historical note: these previously read 6 / 7 / 8 (a leftover from an older
> gait ordering), which made Wave show happy eyes, Pee show angry eyes, and
> Scrape open the weather screen while Info did nothing. Fixed to 7 / 8 / 9.

---

## 5. How gaits work

All cyclic gaits share the same shape (`*Gait::calculate`):

1. **Idle check** — if `|throttle| < 5` and `|yaw| < 5`, reset `phase` to 0,
   hold the standing stance, and allow pitch/roll to tilt the robot in place.
2. **Speed & phase** — `speed = hypot(throttle, yaw)/100`, clamped to 1.0. Phase
   advances by `phaseSpeed * speed * dt`; negative throttle runs it backward.
   The per-gait `phaseSpeed` multiplier sets the cadence (Walk 6 < Trot 10 <
   Gallop 15).
3. **Steering amplitudes** — `leftAmp = throttle + yaw`, `rightAmp = throttle - yaw`
   (differential drive), scaled by a per-gait `maxAmp`.
4. **Leg pairing** — sine waves with per-leg phase offsets define the gait
   character (diagonal trot, 4-beat walk, bounding gallop).
5. **Final angles** — `90 + baseOffset + sin(...) * amp` per leg.

Poses (`StaticPoses.cpp`) skip the phase machinery: Sit/Stretch/Info are fixed
angle sets; Wave/Pee/Scrape use a small phase/timer for their animation.

### Stance offsets (the ±20 fronts)

Trot/Walk stand with the front legs at **+20** (110°) and Gallop at **-20**
(70°). These shift the center of mass to keep the chosen gait stable — they are
empirically tuned, not arbitrary.

---

## 6. AutoGait blending

`AutoGait` morphs continuously between Walk and Gallop as a function of speed.
A `blendFactor` (0 = Walk, 1 = Gallop) linearly interpolates every parameter:
phase speed (6→15), amplitude (35→30), per-leg phase offsets (4-beat→bounding),
and front stance (+20→−20).

Current blend window (tuned): start blending at **0.7** speed, fully Gallop at
**0.9**:

```cpp
if (speed > 0.7f) blendFactor = (speed - 0.7f) / 0.2f;   // 0.2 = window width
```

Below 0.05 speed it idles in the Walk stance. The narrower the window, the
snappier the Walk→Gallop transition feels.

---

## 7. Kinematics: turning a target angle into a servo command

`RobotKinematics::processAngle(index, target, applyLimits)` is the funnel every
angle passes through before hitting hardware:

1. If the servo is **disabled**, return 90 (mechanical center).
2. **Deviation mapping**: `deviation = (target - 90) * invert`, then
   `final = 90 + deviation + offset` (per-servo trim).
3. **Safety clamp** (when `applyLimits`): bound to `90 ± maxAngle`. Walking
   gaits enforce this; poses and active pitch/roll overrides bypass it for
   extreme folding.

### Pitch/roll overrides

In `tick()`, when pitch or roll is pushed while throttle/yaw are near zero, the
engine overrides the gait output to tilt the body (front/hind shifted by a
magnitude derived from pitch). Note the empirical rule documented in the code:
**decreasing a servo angle below 90 moves that leg backward.**

### Folded-pose transition state machine

Standing up directly from Sit would flip the robot backward (CoM is behind the
feet). So `setGait()` detects folded→unfolded transitions, sets a
`pendingGaitIndex`, and `tick()` interpolates the hind legs from folded (30°) to
stretched (150°) over ~0.66s before activating the requested gait.

---

## 8. Configuration & persistence

`PreferencesConfig` stores everything in the ESP32 NVS namespace `"minibot"`:

- Per-servo `pin`, `offset`, `invert` (saved on change from the Web UI).
- OpenWeather `ow_key`, `lat`, `lon`.

On first boot it writes `DEFAULT_SERVO_CONFIGS` and sets an `init` flag;
subsequent boots load the saved values into a RAM cache. Changing a pin requires
a reboot; offset/invert changes apply live (see the deliberate no-reattach note
in `onConfigUpdated()`).

---

## 9. Web UI & WebSocket protocol

`WebUIManager` serves a single self-contained HTML page (embedded as a PROGMEM
string) and talks to the firmware over a WebSocket at `/ws`. The UI uses
nipplejs for the two virtual joysticks:

- **Left stick** → throttle (Y) + yaw (X)
- **Right stick** → pitch (Y) + roll (X)

Browser → firmware messages (JSON, keyed by `type`):

| `type` | Payload | Action |
|--------|---------|--------|
| `joystick` | `{t, y, p, r}` | Update inputs (throttle/yaw/pitch/roll). |
| `set_mode` | `{mode: index}` | Switch gait/pose by index (see §4 table). |
| `get_config` | — | Request current servo + weather settings. |
| `set_config` | `{servos:[{pin,offset,invert}...]}` | Save servo calibration. |
| `set_weather_cfg` | `{ow_key, ow_lat, ow_lon}` | Save weather settings. |

Firmware → browser: a `config` message answering `get_config`.

> Security note: the web/WebSocket server is **unauthenticated** — anyone on the
> same network can drive the robot. Fine for a LAN toy; add auth if that ever
> matters.

---

## 10. OLED display (Core 0)

`DisplayManager` runs `displayTask` pinned to Core 0 at ~20 FPS:

- **Boot splash** for the first 3s (name, IP, loop time).
- **Procedural eyes** with an emotion state machine (idle, wandering, happy,
  angry, blink) that reacts to joystick input and the active pose. Eyes track
  yaw→X / pitch→Y, grow with forward throttle, and wander when idle >3s.
- **Weather screen** (on the Info pose) — fetches OpenWeatherMap every 10
  minutes and renders temp, condition, location, and an animated sun/cloud/rain
  icon.

State is handed from Core 1 via `updateData()` each main-loop iteration.

---

## 11. Main loop timing

`loop()` targets a fixed **~50Hz** cadence (`LOOP_DELAY_MS = 20`):

1. `webUI.loop()` — service WebSocket housekeeping.
2. `ArduinoOTA.handle()` — service OTA updates.
3. `robot.tick()` — compute and write servo angles.
4. Feed `DisplayManager` (Core 0 renders independently).
5. 1Hz serial status log.
6. Sleep the remainder of the 20ms budget.

Keep `tick()` and `loop()` non-blocking. The one intentional blocking spot is
`reattachServos()`, which staggers servo attach with `delay(300)` to avoid a
brownout from all four servos drawing inrush current at once.

---

## 12. Extending: adding a new gait/pose

1. Create `include/<Name>.h` + `src/<Name>.cpp` implementing `IGaitStrategy`
   (`calculate` required; override `applyLimits`/`reset`/`isFoldedPose` as
   needed). Mirror an existing gait for the documentation style.
2. Instantiate it in `main.cpp` and add it to `allGaits[]`.
3. Add a matching `mode-btn` in the Web UI HTML in `WebUIManager.cpp` with the
   **correct index** (array position).
4. If the OLED should react to it, update `DisplayManager.cpp` — and remember
   the index drift in §4.
5. Update the table in §4 of this document.
