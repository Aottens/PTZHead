# Phase 1: Platform Migration and Cleanup - Research

**Researched:** 2026-04-04
**Domain:** ESP32 Arduino platform migration, stepper motor library migration, legacy code removal
**Confidence:** HIGH

## Summary

Phase 1 is a platform migration and cleanup phase with two major technical tasks: (1) replacing the Bluepad32-patched Arduino core with standard espressif32 Arduino framework, and (2) migrating from AccelStepper to FastAccelStepper. The rest is surgical removal of legacy modules (gamepad, WebSocket, ownership) and simplifying main.cpp to a serial-test baseline.

The platform switch is straightforward: remove the `platform_packages` override in `platformio.ini` that pulls in the Bluepad32-patched framework, and remove the WebSockets and ArduinoJson lib_deps. The existing WiFiManager module should work without changes on the standard framework. FastAccelStepper is a well-documented library with hardware-timer-driven stepping on ESP32 (MCPWM/PCNT), native acceleration/deceleration, auto-enable pin management, and continuous-run commands (`runForward`/`runBackward`) that map cleanly to the existing normalized velocity API.

The main risk is that FastAccelStepper uses fundamentally different speed units (Hz, not steps/s as a float) and does not use a polling `run()` loop like AccelStepper -- it is interrupt-driven. The `update()` method's manual slew-rate ramping can be completely removed since FastAccelStepper handles acceleration natively. The velocity mapping pattern changes from "compute target position each frame" to "set speed and call runForward/runBackward or stopMove."

**Primary recommendation:** Replace AccelStepper's position-tracking velocity simulation with FastAccelStepper's native `runForward()`/`runBackward()` + `setSpeedInHz()` + `applySpeedAcceleration()` for continuous velocity control. This eliminates the manual slew ramping and the `run()` polling loop entirely.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Switch from AccelStepper to FastAccelStepper in this phase (not deferred)
- Use FastAccelStepper's built-in acceleration/deceleration -- remove the manual slew rate ramping in ptz_motion's update()
- Keep the normalized -1.0 to 1.0 velocity API -- OSC will send normalized floats from Companion, and speed presets scale the max
- Let FastAccelStepper manage enable pins (active-low) with its auto-enable feature -- remove the manual idle timeout enable/disable logic from main.cpp
- Minimal loop: WiFi + motor test via serial commands
- Serial commands use simple text format: 'PAN 0.5', 'TILT -1.0', 'STOP', etc. -- temporary verification tool
- Keep 'WIFI RESET' serial command for re-provisioning
- Remove preset save/recall logic entirely (g_presets array, Preset struct) -- deferred to v2
- No OSC stub -- that is Phase 2 scope
- Remove all legacy constants: gamepad combos, WebSocket, owner timeout
- Remove kDeadzone and kUseExpo -- analog stick concepts
- Keep kInvertPan as compile-time constant
- Clean up LogRateId enum: remove kLogRateWsStatus, kLogRateWsParseError, kLogRateGamepadCombo
- Manual serial monitor testing only -- no PlatformIO unit tests for this phase

### Claude's Discretion
- FastAccelStepper pin configuration and timer allocation details
- Exact serial command parser implementation
- ptz_motion internal refactor approach for FastAccelStepper integration
- Build flag adjustments for standard espressif32

### Deferred Ideas (OUT OF SCOPE)
None -- discussion stayed within phase scope
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| PLAT-01 | Firmware builds on standard espressif32 Arduino framework (no Bluepad32 patched core) | Remove `platform_packages` override, use `espressif32@6.10.0` or newer; remove Bluepad32 lib_deps |
| PLAT-02 | Bluepad32 gamepad module is fully removed | Delete `ptz_gamepad.cpp`/`.h`, remove all includes and references from `main.cpp` |
| PLAT-03 | WebSocket API module is fully removed | Delete `ptz_ws.cpp`/`.h`, remove WebSockets and ArduinoJson from lib_deps |
| PLAT-04 | Ownership arbitration module is fully removed | Delete `ptz_owner.cpp`/`.h`, remove Owner enum usage from `main.cpp` |
| PLAT-05 | Main loop is simplified for single-source OSC input | Strip dispatch logic, add serial test commands, integrate FastAccelStepper velocity control |
</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| espressif32 (PlatformIO platform) | 6.10.0 (current in platformio.ini) | ESP32 Arduino framework | Standard PlatformIO platform; uses Arduino ESP32 core 2.0.17; no need to change from current version, just remove the Bluepad32 override |
| gin66/FastAccelStepper | ^0.31.0 | Hardware-timer stepper control | ISR-driven stepping at up to 200kHz on ESP32; native acceleration profiles; auto-enable pin support; replaces software-polled AccelStepper |
| tzapu/WiFiManager | ^2.0.17 | WiFi provisioning via captive portal | Already in use, no changes needed |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| (none added) | - | - | Phase 1 removes dependencies, does not add new supporting libraries |

### Removed Dependencies
| Library | Reason |
|---------|--------|
| waspinator/AccelStepper | Replaced by FastAccelStepper |
| links2004/WebSockets | WebSocket module removed (PLAT-03) |
| bblanchon/ArduinoJson | Only used by WebSocket module |
| framework-arduinoespressif32 (Bluepad32 fork) | Replaced by standard framework (PLAT-01) |

**Updated platformio.ini lib_deps:**
```ini
lib_deps =
  tzapu/WiFiManager@^2.0.17
  gin66/FastAccelStepper@^0.31.0
```

**Updated platformio.ini (remove platform_packages entirely):**
```ini
[env:esp32dev]
platform = espressif32@6.10.0
board = esp32dev
framework = arduino

monitor_speed = 115200
monitor_rts = 0
monitor_dtr = 0

; platform_packages line REMOVED (was Bluepad32 patched core)

lib_deps =
  tzapu/WiFiManager@^2.0.17
  gin66/FastAccelStepper@^0.31.0

build_flags =
  -DCORE_DEBUG_LEVEL=0
  -Os
  -ffunction-sections
  -fdata-sections
  -Wl,--gc-sections

build_unflags =
  -O2
  -flto
```

## Architecture Patterns

### Recommended Project Structure (after cleanup)
```
src/
  main.cpp          # Setup + minimal loop (WiFi, serial commands, motor test)
  ptz_config.h      # Pin assignments, motion parameters, log config (cleaned)
  ptz_motion.cpp    # FastAccelStepper-based 3-axis velocity control
  ptz_motion.h      # Public API: setVelocity(), stop(), begin()
  ptz_wifi.cpp      # WiFiManager provisioning (unchanged)
  ptz_wifi.h        # WiFi interface (unchanged)
  ptz_log.cpp       # Rate-limited logging (enum cleaned up)
  ptz_log.h         # Log macros (unchanged)
```

### Pattern 1: FastAccelStepper Velocity Control (replaces AccelStepper position-tracking)

**What:** Instead of computing a target position each frame and calling `run()`, use FastAccelStepper's continuous-run commands with dynamic speed changes.

**When to use:** Any time normalized velocity input (-1.0 to 1.0) must drive a stepper at variable speed.

**Key difference from AccelStepper:** AccelStepper required a `run()` call every loop iteration to advance steps. FastAccelStepper is ISR-driven -- once `runForward()`/`runBackward()` is called, stepping happens in hardware timers. The loop only needs to update speed when input changes.

**Example:**
```cpp
// Source: FastAccelStepper API docs + project context
#include "FastAccelStepper.h"

FastAccelStepperEngine engine = FastAccelStepperEngine();
FastAccelStepper* panStepper = nullptr;

void setup() {
  engine.init();
  panStepper = engine.stepperConnectToPin(ptz::kPanStepPin);
  panStepper->setDirectionPin(ptz::kPanDirPin);
  panStepper->setEnablePin(ptz::kPanEnPin, true);  // true = low-active
  panStepper->setAutoEnable(true);
  panStepper->setDelayToDisable(500);  // 500ms after last step
  panStepper->setAcceleration(20000);  // steps/s^2
}

// Called when normalized velocity changes
void setAxisVelocity(FastAccelStepper* stepper, float normVelocity, float maxSps) {
  if (fabsf(normVelocity) < 0.001f) {
    stepper->stopMove();  // decelerate to stop
    return;
  }
  uint32_t speedHz = static_cast<uint32_t>(fabsf(normVelocity) * maxSps);
  if (speedHz < 1) speedHz = 1;
  stepper->setSpeedInHz(speedHz);
  stepper->applySpeedAcceleration();
  if (normVelocity > 0.0f) {
    stepper->runForward();
  } else {
    stepper->runBackward();
  }
}
```

### Pattern 2: Serial Command Parser (temporary test interface)

**What:** Simple line-based serial parser for motor test commands during validation.

**When to use:** Phase 1 validation only -- replaced by OSC in Phase 2.

**Example:**
```cpp
// Extend existing handleSerialCommands() pattern from main.cpp
void handleSerialCommands() {
  static String line;
  while (Serial.available()) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r') {
      line.trim();
      if (line.length() == 0) { line = ""; continue; }

      if (line.equalsIgnoreCase("STOP")) {
        g_motion.stop();
      } else if (line.equalsIgnoreCase("WIFI RESET")) {
        g_wifi.resetAndProvision();
      } else if (line.startsWith("PAN ") || line.startsWith("pan ")) {
        float val = line.substring(4).toFloat();
        g_motion.setVelocity(val, 0.0f, 0.0f);  // adjust API as needed
      } else if (line.startsWith("TILT ") || line.startsWith("tilt ")) {
        float val = line.substring(5).toFloat();
        g_motion.setVelocity(0.0f, val, 0.0f);
      } else if (line.startsWith("ZOOM ") || line.startsWith("zoom ")) {
        float val = line.substring(5).toFloat();
        g_motion.setVelocity(0.0f, 0.0f, val);
      }
      line = "";
    } else {
      line += c;
    }
  }
}
```

### Anti-Patterns to Avoid
- **Calling run() in a loop with FastAccelStepper:** FastAccelStepper is ISR-driven. There is no `run()` equivalent. Do not poll for step generation. The library handles this via hardware timers (MCPWM/PCNT on ESP32).
- **Position-tracking for velocity control:** The old pattern computed `target += velocity * dt` and called `moveTo()`. With FastAccelStepper, use `runForward()`/`runBackward()` + `setSpeedInHz()` for continuous velocity. Position tracking is not needed for velocity mode.
- **Manual slew-rate ramping:** FastAccelStepper's `setAcceleration()` handles ramp-up/ramp-down. The `kPanSlewSps2` / manual delta clamping in `update()` is redundant and must be removed.
- **Manual enable/disable with idle timeout:** FastAccelStepper's `setAutoEnable(true)` + `setDelayToDisable()` handles this. Remove the `g_idleStartMs` logic and `kIdleDisableTimeoutMs` from main.cpp.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Step pulse generation | Software bit-banging or `run()` polling | FastAccelStepper ISR/hardware timers | Jitter-free stepping up to 200kHz; no CPU load |
| Acceleration/deceleration profiles | Manual slew-rate delta clamping | `setAcceleration()` | Library computes proper trapezoidal profiles |
| Stepper enable pin management | Manual enable/disable with idle timeout | `setAutoEnable(true)` + `setDelayToDisable()` | Handles edge cases, shared enable pins |
| Direction change with deceleration | Checking current direction and stopping first | `runForward()`/`runBackward()` | Library handles decel-stop-reverse automatically |

**Key insight:** AccelStepper required significant application-level code to simulate velocity mode (position tracking, slew ramping, polling). FastAccelStepper has native velocity mode (`runForward`/`runBackward`), making most of `ptz_motion::update()` unnecessary.

## Common Pitfalls

### Pitfall 1: FastAccelStepper speed units are integers, not floats
**What goes wrong:** AccelStepper uses `float` for speed (e.g., `setMaxSpeed(4000.0f)`). FastAccelStepper's `setSpeedInHz()` takes `uint32_t`. Passing 0 is an error.
**Why it happens:** Direct port from AccelStepper code without adapting types.
**How to avoid:** Convert normalized float to uint32_t Hz. Clamp minimum to 1. Use `fabsf()` for absolute value before cast.
**Warning signs:** Compiler warnings about float-to-int conversion; stepper not moving at low speeds.

### Pitfall 2: Speed/acceleration not applied until movement command
**What goes wrong:** Calling `setSpeedInHz()` or `setAcceleration()` does not immediately affect a running stepper.
**Why it happens:** FastAccelStepper requires an explicit call to `applySpeedAcceleration()`, `runForward()`, `runBackward()`, `move()`, or `moveTo()` to apply new values.
**How to avoid:** After changing speed, always call `applySpeedAcceleration()` if the stepper is already running, or call `runForward()`/`runBackward()` which implicitly applies.
**Warning signs:** Speed changes seem to be ignored; stepper continues at old speed.

### Pitfall 3: stepperConnectToPin returns NULL
**What goes wrong:** Engine runs out of stepper slots or pin is invalid. Crash on null dereference.
**Why it happens:** ESP32 has a limited number of hardware timer channels. Default is 6 steppers on ESP32.
**How to avoid:** Always check return value of `stepperConnectToPin()`. Three axes is well within the limit.
**Warning signs:** Null pointer crash during `begin()`.

### Pitfall 4: NVS/WiFi credentials may need re-provisioning
**What goes wrong:** After switching from Bluepad32-patched core to standard espressif32, WiFi credentials stored in NVS may not be found.
**Why it happens:** Different Arduino core builds may use different NVS partition layouts or namespaces.
**How to avoid:** Accept that a one-time re-provision via captive portal may be needed after the platform switch. Document this as expected behavior.
**Warning signs:** Device fails to connect to WiFi after firmware update; falls through to captive portal.

### Pitfall 5: stopMove() decelerates, forceStop() is abrupt
**What goes wrong:** Using `forceStop()` for normal stops causes jerky motion. Using `stopMove()` when immediate stop is needed causes overshoot.
**Why it happens:** Not understanding the two stop modes.
**How to avoid:** Use `stopMove()` for normal velocity-to-zero transitions (smooth deceleration). Reserve `forceStop()` for emergency/watchdog stops only.
**Warning signs:** Motors jerking to a halt; or motors coasting past expected stop point.

### Pitfall 6: setEnablePin active-low parameter
**What goes wrong:** Stepper drivers don't enable/disable correctly.
**Why it happens:** FastAccelStepper's `setEnablePin(pin, low_active_enables_stepper)` second parameter is `true` for active-low (which is the case for this hardware -- pins 25, 26, 27 are active-low per code comments).
**How to avoid:** Pass `true` as second parameter: `setEnablePin(kPanEnPin, true)`.
**Warning signs:** Motors always enabled or always disabled regardless of auto-enable state.

## Code Examples

### Complete PtzMotion class refactored for FastAccelStepper

```cpp
// Source: FastAccelStepper API docs + existing ptz_motion pattern
// ptz_motion.h (refactored)
#pragma once
#include "FastAccelStepper.h"

namespace ptz {

class PtzMotion {
 public:
  void begin();
  void setVelocity(float panNorm, float tiltNorm, float zoomNorm);
  void stop();
  bool isMoving();
  int32_t panPosition();
  int32_t tiltPosition();
  int32_t zoomPosition();

 private:
  void setAxisVelocity(FastAccelStepper* stepper, float norm, float maxSps);

  FastAccelStepperEngine engine_;
  FastAccelStepper* pan_ = nullptr;
  FastAccelStepper* tilt_ = nullptr;
  FastAccelStepper* zoom_ = nullptr;
};

}  // namespace ptz
```

**Key changes from current API:**
- Removed `update(float dt)` -- no longer needed (ISR-driven)
- Removed `run()` -- no longer needed (ISR-driven)
- Removed `setEnabled(bool)` / `enabled()` -- auto-enable handles this
- Removed `moveTo()` -- not needed for velocity-only phase
- Removed `MotionState` struct -- simplified to position queries
- `setVelocity()` now directly drives hardware; no intermediate slew ramping

### FastAccelStepper engine initialization (ESP32-specific)

```cpp
// Source: FastAccelStepper API + ESP32 specifics
void PtzMotion::begin() {
  engine_.init();  // default: uses any available core
  // For pinning to core 1 (leaving core 0 for WiFi): engine_.init(1);

  pan_ = engine_.stepperConnectToPin(kPanStepPin);
  tilt_ = engine_.stepperConnectToPin(kTiltStepPin);
  zoom_ = engine_.stepperConnectToPin(kZoomStepPin);

  // Configure pan
  pan_->setDirectionPin(kPanDirPin, !kInvertPan);  // dirHighCountsUp
  pan_->setEnablePin(kPanEnPin, true);              // active-low
  pan_->setAutoEnable(true);
  pan_->setDelayToDisable(500);                     // 500ms idle before disable
  pan_->setAcceleration(static_cast<int32_t>(kPanAccel));

  // Configure tilt (same pattern)
  tilt_->setDirectionPin(kTiltDirPin);
  tilt_->setEnablePin(kTiltEnPin, true);
  tilt_->setAutoEnable(true);
  tilt_->setDelayToDisable(500);
  tilt_->setAcceleration(static_cast<int32_t>(kTiltAccel));

  // Configure zoom (same pattern)
  zoom_->setDirectionPin(kZoomDirPin);
  zoom_->setEnablePin(kZoomEnPin, true);
  zoom_->setAutoEnable(true);
  zoom_->setDelayToDisable(500);
  zoom_->setAcceleration(static_cast<int32_t>(kZoomAccel));
}
```

### Cleaned ptz_config.h constants (what stays, what goes)

```cpp
// KEEP: Pin assignments
constexpr uint8_t kPanStepPin = 16;   // etc.

// KEEP: Motion parameters (units still valid for FastAccelStepper)
constexpr float kPanMaxSps = 4000.0f;   // max speed in steps/s
constexpr float kPanAccel = 20000.0f;    // acceleration in steps/s^2

// REMOVE: kPanSlewSps2, kTiltSlewSps2, kZoomSlewSps2 (manual ramping gone)
// REMOVE: kDeadzone, kUseExpo (analog stick concepts)
// REMOVE: kGamepadOwnerTimeoutMs
// REMOVE: kWebsocketPort, kWebsocketPath, kProtocolVersion
// REMOVE: kProvisionComboHoldMs, kTakeControlHoldMs, kPresetHoldMs
// REMOVE: kAppHeartbeatTimeoutMs (will be re-added in Phase 2 for OSC watchdog)
// REMOVE: kStatusIntervalMs (will be re-added in Phase 3 for feedback)

// KEEP: kInvertPan
// KEEP: kWifi* constants
// KEEP: kIdleDisableTimeoutMs -- actually REMOVE, replaced by FastAccelStepper setDelayToDisable()

// KEEP: LogLevel, kLogLevel
// CLEAN: LogRateId enum -- remove kLogRateWsStatus, kLogRateWsParseError, kLogRateGamepadCombo, kLogRateOwnerState
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| AccelStepper software polling (`run()` each loop) | FastAccelStepper hardware ISR (MCPWM/PCNT on ESP32) | Library available since ~2020, mature | Eliminates CPU-bound step generation; supports up to 200kHz |
| Manual slew-rate ramping in application code | Library-native `setAcceleration()` | Built into FastAccelStepper | Removes ~40 lines of delta-clamping code from `update()` |
| Bluepad32-patched Arduino core for gamepad | Standard espressif32 Arduino core | This migration | Removes nonstandard framework dependency; standard OTA/library compat |
| Multi-source input dispatch (gamepad + WebSocket + ownership) | Single serial test interface (Phase 1) / OSC (Phase 2) | This migration | Simplifies main loop from ~120 lines to ~30 lines |

**Deprecated/outdated:**
- AccelStepper: Still maintained but polling-based; poor fit for ESP32 with its hardware timer resources
- Bluepad32 framework fork: Active project, but no longer needed since gamepad input is removed

## Open Questions

1. **FastAccelStepper acceleration units vs current config values**
   - What we know: Current `kPanAccel = 20000.0f` is in steps/s^2, and FastAccelStepper `setAcceleration()` also expects steps/s^2 as `int32_t`. Units match.
   - What's unclear: Whether the feel of acceleration is identical, since AccelStepper used the value differently (applied to position targeting vs. FastAccelStepper's direct velocity ramp). The slew constants `kPanSlewSps2 = 6000.0f` also acted as an additional velocity ramp -- removing them will change the feel.
   - Recommendation: Start with current `kPanAccel` values, test via serial, and tune if needed. The slew constants were an extra layer that may have compensated for AccelStepper's position-based approach.

2. **FastAccelStepper timer/channel allocation on ESP32**
   - What we know: ESP32 has 6 MCPWM channels (3 per unit) and 8 RMT channels. FastAccelStepper can use either. Default allocation supports at least 6 steppers.
   - What's unclear: Which driver type (MCPWM_PCNT vs RMT) is best for this 3-axis use case.
   - Recommendation: Use default allocation (let `stepperConnectToPin()` choose). Three steppers is well within limits. Only investigate if issues arise.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Manual serial testing (no automated test framework) |
| Config file | none -- user decision: manual serial monitor testing only |
| Quick run command | `pio run -t upload && pio device monitor` |
| Full suite command | N/A -- manual validation per CONTEXT.md decision |

### Phase Requirements to Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| PLAT-01 | Firmware compiles on standard espressif32 | build | `pio run` | N/A (build system) |
| PLAT-02 | No Bluepad32 gamepad references in source | manual-only | Grep for `bluepad32`, `gamepad`, `Bluepad` in src/ | N/A |
| PLAT-03 | No WebSocket references in source | manual-only | Grep for `WebSocket`, `ptz_ws` in src/ | N/A |
| PLAT-04 | No ownership module references in source | manual-only | Grep for `ptz_owner`, `Owner` in src/ | N/A |
| PLAT-05 | Main loop simplified; serial motor test works | manual-only | Upload + serial: `PAN 0.5`, `TILT -1.0`, `STOP` | N/A |

### Sampling Rate
- **Per task commit:** `pio run` (compile check)
- **Per wave merge:** `pio run -t upload` + manual serial test (all three axes)
- **Phase gate:** Compile succeeds, WiFi connects, all three axes respond to serial commands

### Wave 0 Gaps
None -- user explicitly decided "manual serial monitor testing only -- no PlatformIO unit tests for this phase." No test infrastructure to set up.

## Sources

### Primary (HIGH confidence)
- [FastAccelStepper API documentation](https://github.com/gin66/FastAccelStepper/blob/master/extras/doc/FastAccelStepper_API.md) - Complete API reference: engine init, pin config, speed/accel, movement commands, auto-enable
- [FastAccelStepper Usage Example](https://github.com/gin66/FastAccelStepper/blob/master/examples/UsageExample/UsageExample.ino) - Reference initialization pattern
- [FastAccelStepper Quick Start Guide (DeepWiki)](https://deepwiki.com/gin66/FastAccelStepper/1.2-quick-start-guide) - Setup, pin config, movement patterns
- Existing source code: `platformio.ini`, `ptz_config.h`, `ptz_motion.cpp`, `main.cpp` - Current implementation to migrate from

### Secondary (MEDIUM confidence)
- [PlatformIO espressif32 releases](https://github.com/platformio/platform-espressif32/releases) - Version 6.10.0 uses Arduino ESP32 core 2.0.17; version 6.13.0 also uses 2.0.17
- [FastAccelStepper GitHub repository](https://github.com/gin66/FastAccelStepper) - Library overview, ESP32 support confirmation
- [FastAccelStepper non-blocking discussion](https://github.com/gin66/FastAccelStepper/discussions/161) - Confirmed `applySpeedAcceleration()` pattern for dynamic speed changes

### Tertiary (LOW confidence)
- FastAccelStepper version: library.properties shows 1.2.5 on master, but PlatformIO registry version may differ. Use `^0.31.0` in lib_deps (PlatformIO registry naming) -- **validate with `pio lib search FastAccelStepper` after migration**

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - Platform migration is well-understood (remove override, keep version); FastAccelStepper API thoroughly documented
- Architecture: HIGH - Velocity mode pattern (runForward/runBackward + setSpeedInHz) is explicitly supported and documented
- Pitfalls: HIGH - Based on API documentation, known ESP32 hardware constraints, and analysis of existing code patterns

**Research date:** 2026-04-04
**Valid until:** 2026-05-04 (stable -- ESP32 Arduino ecosystem moves slowly)
