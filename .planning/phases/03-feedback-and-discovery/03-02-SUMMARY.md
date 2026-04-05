---
phase: 03-feedback-and-discovery
plan: 02
subsystem: osc-feedback

tags: [osc, udp, cnmat, esp32, feedback-tx, wifi-rssi]

# Dependency graph
requires:
  - phase: 03-feedback-and-discovery
    provides: per-axis isMoving accessors, activePreset(), kOscAddr* constants, kFeedbackPeriodMs, kLogRateFeedbackTx
  - phase: 02-network-and-core-osc-control
    provides: PtzOsc RX path, OSCMessage send/empty pattern, CNMAT OSC lib
provides:
  - reply-to-sender cache populated on every RX parse
  - updateFeedback() diff + 1s self-heal snapshot loop
  - sendScalarInt helper with int32 type tag and heap hygiene
  - 5 status channels emitted: pan/tilt/zoom moving, preset, rssi
affects: [companion-integration, end-to-end-validation]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Reply-to-sender cache: capture remoteIP/remotePort BEFORE buffer drain"
    - "On-change diff + periodic self-heal snapshot for idempotent TX"
    - "hasSender() gate using port != 0 sentinel to suppress pre-RX emissions"

key-files:
  created: []
  modified:
    - src/ptz_osc.h
    - src/ptz_osc.cpp
    - src/main.cpp

key-decisions:
  - "Preset sentinel = -1 forces first-emit after boot (uint8_t promoted to int32_t)"
  - "Periodic tick re-emits ALL 5 values (including ones just sent via diff) for simplicity"
  - "RSSI gated on WL_CONNECTED to avoid invalid values when link is down"

patterns-established:
  - "TX path never writes lastRxMs_ — heartbeat invariant owned solely by update() parse-success"
  - "Every outbound OSCMessage.send() is followed by msg.empty() to release CNMAT heap"
  - "sendScalarInt wraps beginPacket/send/endPacket/empty as a single unit"

requirements-completed: [FB-01, FB-02, FB-03, FB-04]

# Metrics
duration: 2min
completed: 2026-04-05
---

# Phase 03 Plan 02: OSC Feedback TX Summary

**Reply-to-sender OSC feedback path emitting 5 int32 status channels (pan/tilt/zoom moving, preset, RSSI) on-change plus a 1 Hz self-heal snapshot.**

## Performance

- **Duration:** 2 min
- **Started:** 2026-04-05T18:50:15Z
- **Completed:** 2026-04-05T18:51:44Z
- **Tasks:** 3
- **Files modified:** 3

## Accomplishments
- PtzOsc now caches sender IP+port on every successful RX parse (captured before buffer drain, Pitfall 5)
- updateFeedback() emits per-field int32 deltas immediately on change, plus a full 5-value snapshot every 1000 ms
- Main loop wires updateFeedback() after update(), preserving heartbeat invariant (lastRxMs_ writes unchanged: count = 2)

## Task Commits

Each task was committed atomically:

1. **Task 1: Extend PtzOsc header** - `bcadda2` (feat)
2. **Task 2: Implement sender capture, sendScalarInt, updateFeedback** - `4173da1` (feat)
3. **Task 3: Wire updateFeedback() into main loop** - `ce1b90d` (feat)

## Files Created/Modified
- `src/ptz_osc.h` - Added StatusSnapshot struct, updateFeedback()/hasSender() API, sender cache, feedback state
- `src/ptz_osc.cpp` - Sender capture in update(), sendScalarInt helper, updateFeedback diff+periodic loop
- `src/main.cpp` - Single updateFeedback() call after g_osc.update() in loop()

## Decisions Made
- Used `int32_t preset = -1` as sentinel to force first-emit after boot (since valid presets are 0..kPresetCount-1)
- Periodic 1 Hz snapshot re-emits all 5 values unconditionally — cheaper than tracking which were emitted this tick
- Added `WiFi.status() != WL_CONNECTED` guard before reading RSSI (Pitfall 4)

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Companion generic-osc clients can now subscribe to `/ptz/status/*` on their OSC reply address automatically
- Ready for Plan 03 (mDNS discovery) which was executed in parallel (wave 2)
- End-to-end hardware validation (Phase 04) can now verify feedback round-trip

## Self-Check: PASSED

- src/ptz_osc.h: FOUND
- src/ptz_osc.cpp: FOUND
- src/main.cpp: FOUND
- commit bcadda2: FOUND
- commit 4173da1: FOUND
- commit ce1b90d: FOUND

---
*Phase: 03-feedback-and-discovery*
*Completed: 2026-04-05*
