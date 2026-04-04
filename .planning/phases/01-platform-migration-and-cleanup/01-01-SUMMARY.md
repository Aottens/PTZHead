---
phase: 01-platform-migration-and-cleanup
plan: 01
subsystem: infra
tags: [espressif32, platformio, fastaccelstepper, cleanup]

# Dependency graph
requires: []
provides:
  - Standard espressif32 platform config with FastAccelStepper dependency
  - Clean config header with only pin/motion/wifi/log constants
  - Minimal main.cpp stub ready for FastAccelStepper migration
affects: [01-02-PLAN]

# Tech tracking
tech-stack:
  added: [gin66/FastAccelStepper@^0.31.0]
  patterns: [minimal-stub-then-rebuild]

key-files:
  created: []
  modified:
    - platformio.ini
    - src/ptz_config.h
    - src/main.cpp

key-decisions:
  - "Keep ptz_motion.h included but not driven in main.cpp -- Plan 02 replaces entirely"
  - "Reduce kLogRateCount from 8 to 4, removing legacy WebSocket/Gamepad/Owner log rate entries"

patterns-established:
  - "Clean config: ptz_config.h contains only pin assignments, motion params, wifi config, and log config"

requirements-completed: [PLAT-01, PLAT-02, PLAT-03, PLAT-04]

# Metrics
duration: 2min
completed: 2026-04-04
---

# Phase 01 Plan 01: Legacy Cleanup Summary

**Stripped Bluepad32 gamepad, WebSocket, and ownership modules; migrated to standard espressif32 with FastAccelStepper dependency**

## Performance

- **Duration:** 2 min
- **Started:** 2026-04-04T07:24:51Z
- **Completed:** 2026-04-04T07:27:01Z
- **Tasks:** 2
- **Files modified:** 9 (1 rewritten, 2 cleaned, 6 deleted)

## Accomplishments
- Migrated platformio.ini from Bluepad32 patched core to standard espressif32@6.10.0
- Removed 4 legacy dependencies (AccelStepper, ArduinoJson, WebSockets, Bluepad32 core), added FastAccelStepper
- Deleted 6 legacy module files (gamepad, websocket, ownership)
- Cleaned ptz_config.h of 15+ legacy constants (deadzone, slew rates, websocket, gamepad combos)
- Stripped main.cpp to minimal stub with only wifi and motion includes

## Task Commits

Each task was committed atomically:

1. **Task 1: Migrate platformio.ini and delete legacy module files** - `158e4de` (feat)
2. **Task 2: Clean ptz_config.h, ptz_log enum, and strip main.cpp** - `e259dd6` (feat)

**Plan metadata:** TBD (docs: complete plan)

## Files Created/Modified
- `platformio.ini` - Standard espressif32 config with FastAccelStepper dep
- `src/ptz_config.h` - Pin assignments, motion params, wifi, log config only
- `src/main.cpp` - Minimal stub: serial commands, wifi, motion include
- `src/ptz_gamepad.cpp` - DELETED
- `src/ptz_gamepad.h` - DELETED
- `src/ptz_ws.cpp` - DELETED
- `src/ptz_ws.h` - DELETED
- `src/ptz_owner.cpp` - DELETED
- `src/ptz_owner.h` - DELETED

## Decisions Made
- Keep ptz_motion.h included in main.cpp but not actively driven (no update/run calls) -- Plan 02 replaces the entire motion module with FastAccelStepper
- Build is expected to fail at this point (AccelStepper.h removed from deps but still referenced in ptz_motion.cpp) -- resolved in Plan 02

## Deviations from Plan

None - plan executed exactly as written.

Note: ptz_motion.cpp still references removed constants (kPanSlewSps2 etc.) and AccelStepper.h. This is explicitly expected per the plan's verification section and will be resolved in Plan 02.

## Issues Encountered
None.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Clean slate established for FastAccelStepper migration (Plan 02)
- 8 source files remain: main.cpp, ptz_config.h, ptz_motion.cpp/h, ptz_wifi.cpp/h, ptz_log.cpp/h
- ptz_motion.cpp/h are the only files with legacy AccelStepper references, isolated for Plan 02 rewrite

---
*Phase: 01-platform-migration-and-cleanup*
*Completed: 2026-04-04*

## Self-Check: PASSED
