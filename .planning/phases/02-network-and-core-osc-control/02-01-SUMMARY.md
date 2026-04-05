---
phase: 02-network-and-core-osc-control
plan: 01
subsystem: infra
tags: [esp32, wifi, osc, config, arduino-esp32]

requires:
  - phase: 01-foundation
    provides: ptz_config.h base constants (kPanMaxSps, pins, LogRateId enum), ptz_wifi WiFiManager flow
provides:
  - kOscPort=8000 constexpr constant
  - kHeartbeatTimeoutMs=500 constexpr constant
  - SpeedPreset struct with 3-entry preset table (slow/medium/fast) and kDefaultPresetIndex=1
  - 6 new LogRateId slots for OSC/heartbeat/reconnect logging (kLogRateCount 4 to 7)
  - WiFi power save disabled (WIFI_PS_NONE / setSleep(false)) on connect and reconnect
  - Event-driven WiFi auto-reconnect via ARDUINO_EVENT_WIFI_STA_DISCONNECTED
affects: [02-02-motion-presets, 02-03-osc-module, 04-hardware-validation]

tech-stack:
  added: []
  patterns:
    - "Event-driven WiFi reconnect (non-blocking, no polling loop)"
    - "Re-assert power-save-off after GOT_IP (handles reconnect reset)"
    - "Preset table as constexpr array indexed by preset id"

key-files:
  created: []
  modified:
    - src/ptz_config.h
    - src/ptz_wifi.cpp

key-decisions:
  - "Speed preset scaling 25/60/100% paired with accel 5000/12000/20000 steps/s^2"
  - "Default preset index = 1 (medium) at boot"
  - "Event-driven reconnect over task/polling loop: minimal code, arduino-esp32 native"

patterns-established:
  - "Low-latency WiFi config: WIFI_PS_NONE + setSleep(false) applied at every IP acquisition"
  - "Rate-limited reconnect logging via logShouldEmit(kLogRateWifiReconnect, 2000)"

requirements-completed: [NET-01, NET-02, NET-04, NET-05]

duration: 2min
completed: 2026-04-05
---

# Phase 02 Plan 01: Config Constants + WiFi Hardening Summary

**Added OSC port/heartbeat/speed-preset constants to ptz_config.h and hardened ptz_wifi for low-latency OSC (WIFI_PS_NONE + event-driven auto-reconnect)**

## Performance

- **Duration:** ~2 min
- **Started:** 2026-04-05T08:44:57Z
- **Completed:** 2026-04-05T08:46:19Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments

- OSC/heartbeat constants and full 3-entry SpeedPreset table declared as constexpr in ptz_config.h
- LogRateId enum extended from 4 to 7 entries to cover OSC/heartbeat/reconnect/heap paths
- WiFi power save disabled (WIFI_PS_NONE, setSleep(false)) replacing incorrect WIFI_PS_MIN_MODEM
- Non-blocking event-driven WiFi auto-reconnect via WiFi.onEvent handlers on STA_DISCONNECTED / STA_GOT_IP
- Firmware compiles clean, flash 66.9% / RAM 15.5%

## Task Commits

1. **Task 1: Add OSC/heartbeat/preset constants and LogRateIds** - `5f8d9a7` (feat)
2. **Task 2: Fix WiFi power save and add event-driven reconnect** - `0ad18f0` (fix)

## Files Created/Modified

- `src/ptz_config.h` - Added kOscPort, kHeartbeatTimeoutMs, SpeedPreset struct, kSpeedPresets[3], kDefaultPresetIndex, kPresetCount, 6 new LogRateId slots
- `src/ptz_wifi.cpp` - Replaced WIFI_PS_MIN_MODEM with WIFI_PS_NONE, added registerWifiEventHandlers() invoked from both connect paths

## Decisions Made

- None beyond the plan - preset scaling and handler structure followed plan spec exactly.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- `kOscPort`, `kHeartbeatTimeoutMs`, `kSpeedPresets`, `kDefaultPresetIndex` available for Plan 02-02 (motion presets) and Plan 02-03 (OSC module)
- WiFi layer now satisfies NET-02 low-latency requirement and NET-05 auto-reconnect
- WiFi reconnect behavior still unverified on real hardware (Phase 04 validation)

## Self-Check: PASSED

- FOUND: src/ptz_config.h (modified, includes new constants)
- FOUND: src/ptz_wifi.cpp (modified, includes event handlers)
- FOUND commit: 5f8d9a7
- FOUND commit: 0ad18f0

---
*Phase: 02-network-and-core-osc-control*
*Completed: 2026-04-05*
