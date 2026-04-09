---
phase: 03-feedback-and-discovery
plan: 01
subsystem: motion+config
tags: [motion, config, feedback, mdns, foundations]
dependency-graph:
  requires: []
  provides:
    - "PtzMotion::isPanMoving() / isTiltMoving() / isZoomMoving() const accessors"
    - "PtzMotion::isMoving() const (aggregate OR)"
    - "kFeedbackPeriodMs (1000ms)"
    - "kOscAddr{Pan,Tilt,Zoom}Moving / kOscAddrPreset / kOscAddrRssi"
    - "kMdnsHostname / kMdnsInstanceName / kMdnsServiceType / kMdnsServiceProto"
    - "LogRateId::kLogRateFeedbackTx (7) / kLogRateMdns (8) / kLogRateCount (9)"
  affects:
    - "src/main.cpp (heartbeat call now routes through const isMoving — source-compatible)"
tech-stack:
  added: []
  patterns:
    - "Per-axis state query via FastAccelStepper::isRunning()"
    - "Aggregate query delegates to per-axis methods (single source of truth)"
key-files:
  created: []
  modified:
    - "src/ptz_motion.h"
    - "src/ptz_motion.cpp"
    - "src/ptz_config.h"
decisions:
  - "Aggregate isMoving() delegates to per-axis OR rather than open-coding isRunning checks"
  - "isMoving() upgraded to const (no member mutation — source-compatible with main.cpp heartbeat)"
metrics:
  duration: "3min"
  tasks: 2
  files: 3
  completed: "2026-04-05T18:48:32Z"
---

# Phase 03 Plan 01: Foundations Summary

Per-axis motion-state accessors added to PtzMotion and Phase 3 feedback/mDNS constants (plus 2 new LogRateIds) declared in ptz_config.h; compile-only foundation consumed by Plans 02 and 03.

## What Was Built

**PtzMotion per-axis queries (`src/ptz_motion.h` / `.cpp`):**
- Added `isPanMoving()`, `isTiltMoving()`, `isZoomMoving()` const methods — each delegates to `FastAccelStepper::isRunning()` on the corresponding axis pointer with null-guard.
- Refactored aggregate `isMoving()` into `bool isMoving() const` that returns `isPanMoving() || isTiltMoving() || isZoomMoving()`.
- `const` qualification on `isMoving()` is source-compatible with the existing main.cpp heartbeat call site.

**ptz_config.h Phase 3 additions:**
- `kFeedbackPeriodMs = 1000` — periodic snapshot cadence.
- 5 OSC feedback address constants (FB-04): `/ptz/status/pan/moving`, `/ptz/status/tilt/moving`, `/ptz/status/zoom/moving`, `/ptz/status/preset`, `/ptz/status/rssi`.
- 4 mDNS constants (NET-03): `kMdnsHostname="ptzhead"`, `kMdnsInstanceName="PTZHead"`, `kMdnsServiceType="_osc"`, `kMdnsServiceProto="_udp"`.
- Extended `LogRateId`: `kLogRateFeedbackTx=7`, `kLogRateMdns=8`, `kLogRateCount=9`.

## Verification

- `pio run` exit 0 after each task. Flash 67.5%, RAM 15.5% (no meaningful change from pre-plan).
- All grep acceptance criteria satisfied (per-axis accessors present, const qualifier added, new constants and LogRateIds present with expected values).
- No new compiler warnings.

## Deviations from Plan

None - plan executed exactly as written.

## Decisions Made

- Aggregate isMoving() delegates to per-axis OR (matches pattern in applySpeedPreset)
- isMoving() upgraded to const — verified source-compatible with main.cpp heartbeat call site

## Commits

- `5b150e1` feat(03-01): add per-axis isMoving accessors to PtzMotion
- `750bcd4` feat(03-01): add Phase 3 feedback, mDNS, and LogRateId constants

## Next

Plan 02 consumes kFeedbackPeriodMs, the 5 feedback OSC addresses, kLogRateFeedbackTx, and per-axis isMoving accessors to build the OSC feedback TX diff loop. Plan 03 consumes the mDNS constants and kLogRateMdns to add mDNS advertisement on GOT_IP.

## Self-Check: PASSED
