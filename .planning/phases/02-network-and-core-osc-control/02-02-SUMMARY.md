---
phase: 02-network-and-core-osc-control
plan: 02
subsystem: motion
tags: [motion, speed-preset, fastaccelstepper, api]
requirements_completed: [MOT-06, SPD-01, SPD-02, SPD-03]
dependency_graph:
  requires:
    - "src/ptz_config.h kSpeedPresets (Plan 02-01)"
    - "FastAccelStepper library"
  provides:
    - "PtzMotion per-axis velocity API (setPanVelocity/setTiltVelocity/setZoomVelocity)"
    - "PtzMotion per-axis stop API (stopPan/stopTilt/stopZoom)"
    - "PtzMotion speed preset API (applySpeedPreset, activePreset)"
  affects:
    - "src/ptz_motion.h"
    - "src/ptz_motion.cpp"
tech_stack:
  added: []
  patterns:
    - "Effective-max-sps state cached per axis, rewritten on preset change"
    - "applySpeedAcceleration() invoked on running steppers for immediate ramp update"
key_files:
  created: []
  modified:
    - "src/ptz_motion.h"
    - "src/ptz_motion.cpp"
decisions:
  - "setVelocity(pan,tilt,zoom) bulk method preserved but routed through *Eff_ values so serial debug commands honor the active preset"
  - "applySpeedPreset invoked at end of begin() to seed *Eff_ members (overrides base setAcceleration() done earlier in begin())"
metrics:
  duration_min: 1
  tasks_completed: 2
  files_touched: 2
  completed_date: "2026-04-05"
---

# Phase 02 Plan 02: PtzMotion Speed Preset & Per-Axis API Summary

Extended PtzMotion with per-axis velocity setters, per-axis stops, and a preset-switching API (applySpeedPreset) that rewrites both max speed and acceleration for all three axes while preserving FastAccelStepper's MOT-06 smooth-accel mechanism.

## What Was Built

**Header (`src/ptz_motion.h`):**
- Added public members: `setPanVelocity(float)`, `setTiltVelocity(float)`, `setZoomVelocity(float)`, `stopPan()`, `stopTilt()`, `stopZoom()`, `applySpeedPreset(uint8_t)`, `activePreset() const`.
- Added private state: `panMaxSpsEff_`, `tiltMaxSpsEff_`, `zoomMaxSpsEff_`, `activePreset_`.
- Added explicit `#include <stdint.h>`.

**Implementation (`src/ptz_motion.cpp`):**
- `applySpeedPreset(idx)` clamps out-of-range input to `kDefaultPresetIndex`, updates effective max-sps per axis (base × scale), writes `setAcceleration()` per axis, and calls `applySpeedAcceleration()` on any stepper that is currently running so new ramps take effect immediately. Emits `PTZ_LOGI("MOTION", "preset=%u maxSps pan=%.0f tilt=%.0f zoom=%.0f", ...)`.
- `begin()` now invokes `applySpeedPreset(kDefaultPresetIndex)` before the final log line to seed state.
- `setVelocity(pan,tilt,zoom)` rerouted to use `panMaxSpsEff_`/`tiltMaxSpsEff_`/`zoomMaxSpsEff_` instead of raw `kPanMaxSps`/`kTiltMaxSps`/`kZoomMaxSps`, so legacy serial debug commands honor the active preset.
- `setAxisVelocity()` private helper unchanged (MOT-06 `applySpeedAcceleration()` preserved on every velocity change).

## Requirements Satisfied

- **MOT-06** — `setAxisVelocity()` still calls `applySpeedAcceleration()` on every velocity change; untouched.
- **SPD-01** — Three presets indexable via `applySpeedPreset(0/1/2)`; out-of-range clamped to default (index 1).
- **SPD-02** — Every preset application writes both max-sps (via `*Eff_`) and `setAcceleration()` for all 3 axes.
- **SPD-03** — `activePreset_` persists in PtzMotion until the next `applySpeedPreset()` call; exposed via `activePreset()` getter.

## Verification

- `pio run` clean build: **SUCCESS** in 2.95s.
- Flash 66.9% (877509 B), RAM 15.5% (50664 B).
- No unused-member warnings, no implicit conversions flagged.
- All grep acceptance criteria for Task 1 (9 matches ≥ 8 required) and Task 2 (8 matches ≥ 7 required) pass.
- Phase 1 public API preserved: `setVelocity`, `stop`, `isMoving`, `panPosition`, `tiltPosition`, `zoomPosition` all remain.

## Deviations from Plan

None - plan executed exactly as written.

## Commits

| Task | Name                                                | Commit  | Files              |
| ---- | --------------------------------------------------- | ------- | ------------------ |
| 1    | Extend PtzMotion header (per-axis + preset API)     | fe234d7 | src/ptz_motion.h   |
| 2    | Implement per-axis setters and applySpeedPreset     | 6c8db6f | src/ptz_motion.cpp |

## Hardware Verification Deferred

Runtime boot log line `preset=1 maxSps pan=2400 tilt=2400 zoom=2400` is producible but not verified on real hardware (Phase 04 scope, per project roadmap).

## Self-Check: PASSED

- src/ptz_motion.h: FOUND
- src/ptz_motion.cpp: FOUND
- Commit fe234d7: FOUND
- Commit 6c8db6f: FOUND
