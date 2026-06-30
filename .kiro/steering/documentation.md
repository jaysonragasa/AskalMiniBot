---
inclusion: always
---

# Documentation Conventions

AskalMiniBot is heavily and deliberately commented. The comments are part of the product: they explain not just *what* the code does but *why* a given angle, multiplier, or timing was chosen. Match the existing style exactly when adding or editing code. Never strip existing comments when refactoring; update them to stay accurate.

## Two-layer model

Every translation unit follows the same two-layer documentation pattern. Keep both layers in sync.

1. **Header (`.h`) — API contract, Doxygen style.** Describe the public surface: what the class is for and what each method/param/return means. A reader should understand how to *use* the type without opening the `.cpp`.
2. **Source (`.cpp`) — implementation narrative, banner style.** Explain how and why the algorithm works, step by step, including empirical tuning values and hardware quirks.

## Header style (Doxygen)

Use `/** ... */` blocks with Doxygen tags. Document the class once, then every public method, and annotate non-obvious member variables with `///<` trailing comments.

- Class: `@class <Name>` + `@brief` one-liner, followed by a short paragraph of purpose/context when the role isn't obvious.
- Methods: `@brief`, then `@param <name>` for each argument and `@return` when it returns a value. Note units and value ranges (e.g. "-100 to 100", "in degrees", "delta time in seconds").
- Members: trailing `///< description` on the same line.

```cpp
/**
 * @class TrotGait
 * @brief Implements a moderate-speed trotting motion for the robot.
 *
 * In a trot, diagonal pairs of legs move together in sync ...
 */
class TrotGait : public IGaitStrategy {
public:
    /**
     * @brief Calculates the target servo angles for the next frame.
     * @param dt Delta time in seconds since the last frame.
     * @param inputs Current joystick inputs for steering and speed.
     * @param servoAngles Array of 4 integers where the resulting angles are written.
     */
    void calculate(float dt, const JoystickData& inputs, int servoAngles[4]) override;
private:
    float phase; ///< The current progress of the trot cycle (0 to 2*PI).
};
```

## Source style (banner comments)

Every function in a `.cpp` is preceded by a banner identifying it. The banner is a line of dashes, the `Class::method` name, and a one-line summary:

```cpp
// -------------------------------------------------------------------------
// TrotGait::calculate
// Computes servo angles for a diagonal trotting motion.
// -------------------------------------------------------------------------
```

Inside longer functions, break the logic into **numbered steps**, each introduced by its own banner block. This is the dominant pattern across the gaits and kinematics:

```cpp
    // -------------------------------------------------------------------------
    // 2. SPEED AND PHASE CALCULATION
    // Calculate speed magnitude and advance the phase.
    // Trot is a medium speed gait (multiplier 10.0f).
    // -------------------------------------------------------------------------
```

Short functions (constructors, trivial getters/setters) get just the top banner with no inner steps. Some plain getters/setters (e.g. the lat/lon accessors in `PreferencesConfig.cpp`) have no banner at all — keep them terse rather than over-documenting.

## What comments must explain (the "why")

Inline comments justify the magic numbers and design decisions. This is the most important convention — preserve and extend it. Examples from the codebase:

- **Tuned constants**: `float phaseSpeed = 10.0f * speed;` is annotated as the medium-speed trot multiplier; gallop is `15.0f`, walk is `6.0f`. State *why* a value was picked (`maxAmp = 30.0f; // Reduced from 45.0f for stability`).
- **Coordinate/kinematic conventions**: the 90° logical center, the `+20` / `-20` front-leg stance offsets, and notes like `// DECREASING angle (< 90) moves leg BACKWARD` derived from empirical observation.
- **Hardware quirks and safety**: `delay(300)` staggering to avoid brownout, ESP32 PWM timer allocation, 500–2500µs pulse widths, and why `onConfigUpdated()` deliberately skips reattaching servos (slider responsiveness).
- **Concurrency**: the FreeRTOS display task is pinned to Core 0 with an explanation that I2C delays must not jitter the Core 1 servo PWM.
- **Phase math**: gait files spell out the leg-pairing logic (e.g. trot diagonal pairs 180° apart, walk 4-beat at PI/2 offsets, gallop hind legs offset by 3*PI/4).

When you introduce a new tuned value, document it the same way: include the rationale and, if you changed it, what it was before and why.

## Adding new code

- **New gait/pose**: header gets the full Doxygen `@class`/`@brief` block plus documented overrides (`calculate`, `applyLimits`, `reset`, `isFoldedPose`). The `.cpp` gets the per-function banners and numbered steps. Mirror an existing gait such as `TrotGait` or `WalkGait`.
- **New method on an existing class**: add the `@brief`/`@param`/`@return` block in the header and a matching banner in the source.
- Keep comments factual and current. A wrong comment is worse than none.

## Gait index alignment (resolved)

`DisplayManager.cpp` keys its emotion/weather logic off gait indices. These were
once stale (read 6/7/8 against an older ordering) but are now aligned with
`main.cpp`'s `allGaits[]`: Pee=7 (HAPPY), Scrape=8 (ANGRY), Info=9 (weather).
When you touch gait indexing or this emotion/weather logic, reconcile any magic
numbers against `allGaits[]` and keep both the values and their comments in sync.
