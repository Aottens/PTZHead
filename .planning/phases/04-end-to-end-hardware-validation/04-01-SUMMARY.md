---
phase: 04-end-to-end-hardware-validation
plan: 01
subsystem: testing
tags: [hardware-validation, companion, streamdeck, osc, platformio]

# Dependency graph
requires:
  - phase: 03-feedback-and-discovery
    provides: Complete firmware with feedback and mDNS
provides:
  - 7-stage hardware test checklist (TEST-CHECKLIST.md)
  - 15-button Companion StreamDeck configuration guide (COMPANION-SETUP.md)
  - Verified firmware compiles cleanly (69.7% flash, 16.1% RAM)
affects: [04-02-PLAN]

# Tech tracking
tech-stack:
  added: []
  patterns: [manual-hardware-test-checklist, companion-generic-osc-button-config]

key-files:
  created:
    - .planning/phases/04-end-to-end-hardware-validation/TEST-CHECKLIST.md
    - .planning/phases/04-end-to-end-hardware-validation/COMPANION-SETUP.md
  modified: []

key-decisions:
  - "D-pad layout with zoom right, presets surrounding D-pad, status displays in column 5"
  - "100ms repeat interval on all motion buttons for heartbeat watchdog compatibility"
  - "Optional Stage 8 (heap stability) included per Claude's discretion on CNMAT concern"

patterns-established:
  - "Press+release pattern: motion buttons send velocity on press (100ms repeat), 0.0 on release as redundant safety"
  - "Feedback-driven button colors: green background when axis moving, preset active"

requirements-completed: [PLAT-05, MOT-01, MOT-02, MOT-03, MOT-04, MOT-05, MOT-06, MOT-07, SPD-01, SPD-02, SPD-03, NET-01, NET-02, NET-03, NET-04, NET-05, FB-01, FB-02, FB-03, FB-04]

# Metrics
duration: 4min
completed: 2026-04-06
---

# Phase 4 Plan 01: Test Preparation Summary

**7-stage hardware test checklist, 15-button Companion StreamDeck config guide, and verified clean firmware build (69.7% flash, 16.1% RAM)**

## Performance

- **Duration:** 4 min
- **Started:** 2026-04-06T17:36:41Z
- **Completed:** 2026-04-06T17:40:12Z
- **Tasks:** 3
- **Files created:** 2

## Accomplishments
- Created comprehensive TEST-CHECKLIST.md mapping all 7 Phase 4 success criteria (SC-1 through SC-7) to concrete test stages with exact commands, expected outputs, and fix hints for known pitfalls
- Created COMPANION-SETUP.md with button-by-button configuration for a 15-button StreamDeck including OSC paths, values, repeat intervals, feedback setup, and layout diagram
- Verified firmware compiles cleanly via `pio run` -- 69.7% flash, 16.1% RAM, matching Phase 3 handoff expectations

## Task Commits

Each task was committed atomically:

1. **Task 1: Create hardware test checklist** - `f92aa37` (feat)
2. **Task 2: Create Companion StreamDeck button configuration guide** - `98580f1` (feat)
3. **Task 3: Verify firmware compiles** - No commit (build verification only, no file changes)

## Files Created/Modified
- `.planning/phases/04-end-to-end-hardware-validation/TEST-CHECKLIST.md` - 7-stage test checklist with 34 pass/fail checkboxes, serial commands, OSC addresses, fix hints
- `.planning/phases/04-end-to-end-hardware-validation/COMPANION-SETUP.md` - 15-button StreamDeck layout with exact Companion generic-osc configuration for all buttons

## Decisions Made
- Used the "alternative/more natural D-pad" layout from research: D-pad center-left, zoom right side, presets surrounding D-pad, status displays in column 5
- All motion buttons configured with 100ms repeat interval -- critical for heartbeat watchdog compatibility (500ms timeout)
- Included optional Stage 8 (heap stability monitoring) per Claude's discretion to address CNMAT/OSC heap concern from Phase 2
- Release action sends 0.0 on all motion buttons as redundant stop safety (heartbeat is primary stop mechanism)

## Deviations from Plan

None -- plan executed exactly as written.

## Issues Encountered

- PlatformIO CLI (`pio`) not on PATH -- resolved by using full path `~/.platformio/penv/bin/pio`. Build succeeded with benign CNMAT/OSC manifest parse warning (known issue, not a compiler warning).

## User Setup Required

None -- no external service configuration required. The COMPANION-SETUP.md guide provides all instructions the user needs to configure Companion manually.

## Next Phase Readiness
- All test artifacts ready for Plan 02 hardware validation execution
- Firmware confirmed to compile cleanly -- ready for `pio run --target upload`
- User has checklist to follow systematically and Companion button config to build the StreamDeck page
- Phase 3 handoff verification items (mDNS resolve, feedback latency, RSSI cadence) are embedded in the test checklist

## Self-Check: PASSED

- All 3 files exist (TEST-CHECKLIST.md, COMPANION-SETUP.md, 04-01-SUMMARY.md)
- Both task commits verified (f92aa37, 98580f1)

---
*Phase: 04-end-to-end-hardware-validation*
*Completed: 2026-04-06*
