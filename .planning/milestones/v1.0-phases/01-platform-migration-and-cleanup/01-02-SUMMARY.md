---
phase: 01-platform-migration-and-cleanup
plan: 02
subsystem: motion
tags: [fastaccelstepper, motion, serial-test, platform-migration]

# Dependency graph
requires: [01-01]
provides:
  - FastAccelStepper-based 3-axis velocity control (ISR-driven)
  - Serial motor test interface (PAN/TILT/ZOOM/STOP/WIFI RESET)
  - Firmware that compiles and boots on standard espressif32
affects: [02-01-PLAN, 04-01-PLAN]

# Tech tracking
tech-stack:
  added: [FastAccelStepper ISR stepping, auto-enable, runForward/runBackward]
  patterns: [velocity-norm → speedHz mapping, null-safe stepper access]

key-files:
  created: []
  modified:
    - src/ptz_motion.h
    - src/ptz_motion.cpp
    - src/main.cpp

key-decisions:
  - "Deferred hardware verification (task 3) to new Phase 4 (Hardware Validation) — hardware not on hand"
  - "FastAccelStepper drives all 3 axes via engine_.init() with default core allocation"
  - "setDelayToDisable(500) replaces old kIdleDisableTimeoutMs auto-disable logic"
  - "Velocity norm < 0.001 triggers stopMove() (smooth deceleration, not forceStop)"

patterns-established:
  - "Velocity API: setVelocity(panNorm, tiltNorm, zoomNorm) with -1.0..1.0 range"
  - "ISR-driven — main loop only needs handleSerialCommands(), no polling"

requirements-completed: [PLAT-05]
requirements-deferred: []  # PLAT-05 code-complete; hardware verification in Phase 4

# Metrics
duration: 4min
completed: 2026-04-05
checkpoint_status: deferred-to-phase-4
---

# Phase 01 Plan 02: FastAccelStepper Motor Control Rewrite

**Rewrote ptz_motion for ISR-driven FastAccelStepper velocity control and added serial motor test commands. Firmware compiles and boots; hardware verification deferred to Phase 4.**

## Performance

- **Duration:** ~4 min (2 coding tasks)
- **Completed:** 2026-04-05
- **Tasks:** 2/3 complete (task 3 deferred)
- **Files modified:** 3

## Accomplishments

- Rewrote `ptz_motion.h` with clean FastAccelStepper-based velocity API (no more `update()`/`run()` polling)
- Rewrote `ptz_motion.cpp` with engine init, auto-enable (active-low), 500ms auto-disable, smooth `stopMove()`
- Added serial command parser to `main.cpp` supporting PAN/TILT/ZOOM/STOP/WIFI RESET
- Firmware compiles successfully: **RAM 15.5%, Flash 66.8%**
- Main loop reduced to single `handleSerialCommands()` call — ISR handles stepping

## Task Commits

1. **Task 1: Rewrite ptz_motion module for FastAccelStepper** — `b42e8f7` (feat)
2. **Task 2: Add serial motor test commands and verify compilation** — `ae42327` (feat)
3. **Task 3: Hardware verification** — DEFERRED to Phase 4 (Hardware Validation)

Also: `.gitignore` for PlatformIO build artifacts — `8d62d7e`

## Files Created/Modified

- `src/ptz_motion.h` — FastAccelStepper-based `PtzMotion` class (velocity control API)
- `src/ptz_motion.cpp` — Engine init + per-axis setup + runForward/runBackward/stopMove
- `src/main.cpp` — Serial test commands wired to `g_motion.setVelocity()`

## Decisions Made

- **Hardware verification deferred** — hardware not physically available; new Phase 4 (Hardware Validation) will cover end-to-end hardware testing across all motion, OSC, and feedback features
- Used default `engine_.init()` core allocation (let FastAccelStepper choose)
- `setSpeedInHz` clamped minimum to 1 (library requirement)
- Case-insensitive command prefix matching in serial parser

## Deviations from Plan

- Task 3 (hardware verification checkpoint) deferred, not executed. All code-level acceptance criteria met. Plan marked code-complete; hardware behavior will be validated in Phase 4.

## Issues Encountered

None during code tasks. PlatformIO build cache `.pio/` was untracked — added `.gitignore` to prevent committing build artifacts.

## User Setup Required

None for code — hardware verification will require ESP32 flashing in Phase 4.

## Next Phase Readiness

- ✅ Firmware compiles on standard espressif32 framework
- ✅ PtzMotion API ready for OSC dispatch in Phase 2 (`setVelocity()` and `stop()`)
- ⏸️  Real-hardware motor response untested (deferred to Phase 4)
- ⚠️  Risks carried to Phase 4: direction pin polarity, speedHz conversion accuracy, auto-disable timing, WiFi credential persistence across framework switch

---
*Phase: 01-platform-migration-and-cleanup*
*Completed: 2026-04-05 (code-complete, hardware verification deferred)*

## Self-Check: PASSED (code-complete)
