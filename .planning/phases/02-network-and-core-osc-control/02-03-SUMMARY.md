---
phase: 02-network-and-core-osc-control
plan: 03
subsystem: network
tags: [osc, cnmat, udp, wifiudp, heartbeat-watchdog, esp32, platformio]

# Dependency graph
requires:
  - phase: 02-network-and-core-osc-control
    provides: WiFi hardening (02-01), PtzMotion per-axis API + speed presets (02-02)
provides:
  - PtzOsc module that listens on UDP port 8000 and dispatches CNMAT/OSC messages
  - /ptz/{pan,tilt,zoom} velocity endpoints bridging to PtzMotion setters
  - /ptz/stop and /ptz/{pan,tilt,zoom}/stop endpoints
  - /ptz/speed/preset integer endpoint
  - 500ms heartbeat watchdog auto-stopping motion on OSC silence
  - Rate-limited heap monitoring log in main loop
affects: [phase-03-feedback-and-discovery, phase-04-hardware-validation]

# Tech tracking
tech-stack:
  added: [CNMAT/OSC@3.5.8 (GitHub tag)]
  patterns:
    - "File-static trampoline pattern for raw function-pointer dispatch callbacks"
    - "Drain-all-queued-UDP-packets per loop iteration"
    - "Heartbeat watchdog keyed on PtzMotion::isMoving() gate"

key-files:
  created:
    - src/ptz_osc.h
    - src/ptz_osc.cpp
  modified:
    - platformio.ini
    - src/main.cpp

key-decisions:
  - "Pulled CNMAT/OSC via GitHub tag URL (#3.5.8) because the PlatformIO registry only exposes v1.0.0"
  - "Heartbeat only fires while g_motion.isMoving() is true, keeping idle device silent"
  - "Unknown-address detection performed post-dispatch via fullMatch checks (CNMAT v3.5.8 exposes no public dispatched flag)"

patterns-established:
  - "s_self file-static pattern: capturing lambdas are impossible with CNMAT dispatch(), so trampolines reach state via a module-scope self pointer"
  - "Rate-limited logging via logShouldEmit on every high-frequency path (velocity rx, parse error, unknown addr, heartbeat, heap)"

requirements-completed: [MOT-01, MOT-02, MOT-03, MOT-04, MOT-05, MOT-07, NET-04]

# Metrics
duration: 2min
completed: 2026-04-05
---

# Phase 02 Plan 03: OSC Receive Module & Heartbeat Summary

**WiFiUDP-based CNMAT/OSC receiver on port 8000 dispatching /ptz/{pan,tilt,zoom,stop,speed/preset} to PtzMotion with 500ms heartbeat auto-stop**

## Performance

- **Duration:** ~2 min
- **Started:** 2026-04-05T08:51:16Z
- **Completed:** 2026-04-05T08:53:24Z
- **Tasks:** 3
- **Files modified:** 4 (2 created, 2 modified)

## Accomplishments
- PtzOsc module created: drains all queued UDP packets per loop, parses OSC via CNMAT, dispatches 8 addresses
- Per-axis velocity, per-axis stop, all-stop, and speed preset endpoints wired end-to-end to PtzMotion
- 500ms heartbeat watchdog auto-stops motion only while moving (idle device silent)
- Rate-limited logging on all hot paths (velocity rx 500ms, parse error 1s, unknown addr 2s, heartbeat 2s, heap 60s)
- Full clean build succeeds with CNMAT/OSC, FastAccelStepper, and WiFiManager linked; 884KB flash fits default 1.25MB partition

## Task Commits

Each task was committed atomically:

1. **Task 1: Add CNMAT/OSC dependency to platformio.ini** - `6bb1b88` (chore)
2. **Task 2: Create ptz_osc module (header + implementation)** - `8bc0619` (feat)
3. **Task 3: Wire ptz_osc into main.cpp with heartbeat watchdog** - `87de759` (feat)

## Files Created/Modified
- `src/ptz_osc.h` - PtzOsc class interface (begin/update/lastRxMs/motionPtr)
- `src/ptz_osc.cpp` - WiFiUDP receive loop, CNMAT dispatch, motion bridge trampolines
- `platformio.ini` - CNMAT/OSC@3.5.8 dependency via GitHub tag
- `src/main.cpp` - PtzOsc instance, begin after wifi, update + heartbeat + heap log in loop

## Decisions Made
- **CNMAT/OSC source:** PlatformIO registry only hosts cnmat/OSC v1.0.0. Research pinned ^3.5.8 (upstream GitHub tag). Switched lib_deps entry from `cnmat/OSC@^3.5.8` to `https://github.com/CNMAT/OSC.git#3.5.8` to honor the pinned version.
- **Heartbeat gate:** Watchdog condition checks `g_motion.isMoving()` before logging or calling `stop()`, preventing log spam and redundant stop calls while the device is idle.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] CNMAT/OSC registry lookup failed with version `^3.5.8`**
- **Found during:** Task 2 (first `pio run` after creating ptz_osc files)
- **Issue:** `UnknownPackageError: Could not find the package with 'cnmat/OSC @ ^3.5.8'`. The PlatformIO library registry only publishes `cnmat/OSC` at version 1.0.0; the 3.5.8 tag exists only on upstream GitHub. The plan (and research) pinned `^3.5.8`, which the registry cannot resolve.
- **Fix:** Changed the `lib_deps` entry from `cnmat/OSC@^3.5.8` to `https://github.com/CNMAT/OSC.git#3.5.8`. Same version, retrieved directly from the upstream tag.
- **Files modified:** platformio.ini
- **Verification:** `pio run` resolves and compiles OSCMessage/OSCBundle from the cloned repo; build succeeds.
- **Committed in:** 8bc0619 (Task 2 commit)

---

**Total deviations:** 1 auto-fixed (1 blocking)
**Impact on plan:** Purely a dependency-sourcing workaround; functional behavior matches research spec. No scope creep.

## Issues Encountered
- None beyond the registry deviation above.

## User Setup Required

None - no external service configuration required. Plan 02-01 already handles WiFi provisioning; OSC listens on port 8000 automatically once WiFi connects.

## Next Phase Readiness
- Phase 02 complete end-to-end: WiFi → OSC receive → per-axis motion with speed presets → heartbeat watchdog
- Phase 03 (feedback/discovery) can consume `g_osc.lastRxMs()` and PtzMotion state for outbound telemetry
- Phase 04 (hardware validation) will verify latency, packet drain behaviour, heartbeat timing, and preset accel accuracy on real hardware

## Self-Check: PASSED

- src/ptz_osc.h: FOUND
- src/ptz_osc.cpp: FOUND
- platformio.ini modified: FOUND
- src/main.cpp modified: FOUND
- Commit 6bb1b88: FOUND
- Commit 8bc0619: FOUND
- Commit 87de759: FOUND
- `pio run` build: SUCCESS (884KB / 1.25MB)

---
*Phase: 02-network-and-core-osc-control*
*Completed: 2026-04-05*
