# Phase 1: Platform Migration and Cleanup - Context

**Gathered:** 2026-04-04
**Status:** Ready for planning

<domain>
## Phase Boundary

Remove legacy input modules (Bluepad32 gamepad, WebSocket, ownership arbitration), switch from Bluepad32-patched Arduino core to standard espressif32 framework, migrate from AccelStepper to FastAccelStepper, and validate the baseline: firmware compiles, boots, connects to WiFi, and motors respond to serial test commands.

</domain>

<decisions>
## Implementation Decisions

### Stepper library
- Switch from AccelStepper to FastAccelStepper in this phase (not deferred)
- Use FastAccelStepper's built-in acceleration/deceleration — remove the manual slew rate ramping in ptz_motion's update()
- Keep the normalized -1.0 to 1.0 velocity API — OSC will send normalized floats from Companion, and speed presets scale the max
- Let FastAccelStepper manage enable pins (active-low) with its auto-enable feature — remove the manual idle timeout enable/disable logic from main.cpp

### Post-cleanup main loop
- Minimal loop: WiFi + motor test via serial commands
- Serial commands use simple text format: 'PAN 0.5', 'TILT -1.0', 'STOP', etc. — temporary verification tool
- Keep 'WIFI RESET' serial command for re-provisioning
- Remove preset save/recall logic entirely (g_presets array, Preset struct) — deferred to v2 (POS-01/02/03)
- No OSC stub — that's Phase 2's scope

### Config cleanup
- Remove all legacy constants: gamepad combos (kProvisionComboHoldMs, kTakeControlHoldMs, kPresetHoldMs), WebSocket (kWebsocketPort, kWebsocketPath, kProtocolVersion), owner timeout (kGamepadOwnerTimeoutMs)
- Remove kDeadzone and kUseExpo — analog stick concepts, not applicable to OSC input
- Keep kInvertPan as compile-time constant — runtime config is v2 (CFG-01)
- Clean up LogRateId enum: remove kLogRateWsStatus, kLogRateWsParseError, kLogRateGamepadCombo. New OSC-relevant IDs added in Phase 2

### Validation method
- Baseline validated = compile on standard espressif32 + boot + WiFi connect + serial motor test (all three axes)
- Re-provisioning WiFi via captive portal is acceptable if NVS layout changes during platform switch
- Manual serial monitor testing only — no PlatformIO unit tests for this phase

### Claude's Discretion
- FastAccelStepper pin configuration and timer allocation details
- Exact serial command parser implementation
- ptz_motion internal refactor approach for FastAccelStepper integration
- Build flag adjustments for standard espressif32

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Platform and framework
- `platformio.ini` — Current Bluepad32-patched framework config that must be replaced with standard espressif32
- `src/ptz_config.h` — All pin assignments (preserved), legacy constants (to remove), and motion parameters

### Modules to remove
- `src/ptz_gamepad.cpp` / `src/ptz_gamepad.h` — Bluepad32 gamepad module (full removal)
- `src/ptz_ws.cpp` / `src/ptz_ws.h` — WebSocket API module (full removal)
- `src/ptz_owner.cpp` / `src/ptz_owner.h` — Ownership arbitration module (full removal)

### Modules to refactor
- `src/ptz_motion.cpp` / `src/ptz_motion.h` — AccelStepper to FastAccelStepper migration, remove manual slew ramping
- `src/main.cpp` — Strip gamepad/ws/owner dispatch, add serial motor test commands

### Modules to preserve
- `src/ptz_wifi.cpp` / `src/ptz_wifi.h` — WiFiManager provisioning (keep as-is)
- `src/ptz_log.cpp` / `src/ptz_log.h` — Rate-limited logging (keep, clean up enum)

### Requirements
- `.planning/REQUIREMENTS.md` — PLAT-01 through PLAT-05 define Phase 1 scope

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `ptz_motion` module: Proven 3-axis motion with normalized velocity API — refactor internals for FastAccelStepper, keep public API
- `ptz_wifi` module: WiFiManager captive portal provisioning — no changes needed
- `ptz_log` module: Rate-limited serial logging with configurable levels — keep, just clean up enum IDs
- Serial command handler in main.cpp: Existing pattern for 'WIFI RESET' — extend for motor test commands

### Established Patterns
- Namespace `ptz::` used consistently across all modules
- Config constants in `ptz_config.h` with `constexpr` — continue this pattern
- Motion module uses normalized -1.0 to 1.0 input, scales internally to steps/second
- Enable pins are active-low on all three axes (pins 25, 26, 27)

### Integration Points
- `platformio.ini` lib_deps: Remove WebSockets, add FastAccelStepper, remove Bluepad32 framework override
- `main.cpp` includes and globals: Remove gamepad/ws/owner includes and instances
- `ptz_config.h`: Pin assignments stay, motion parameters may need adjustment for FastAccelStepper units

</code_context>

<specifics>
## Specific Ideas

No specific requirements — open to standard approaches

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 01-platform-migration-and-cleanup*
*Context gathered: 2026-04-04*
