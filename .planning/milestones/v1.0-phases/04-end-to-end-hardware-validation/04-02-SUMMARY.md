---
phase: 04-end-to-end-hardware-validation
plan: 02
subsystem: firmware, testing
tags: [esp32, osc, companion, streamdeck, mdns, stepper, hardware-validation]

requires:
  - phase: 04-end-to-end-hardware-validation/04-01
    provides: TEST-CHECKLIST.md, COMPANION-SETUP.md, verified build
  - phase: 03-feedback-and-discovery
    provides: OSC feedback TX, mDNS advertisement, heartbeat watchdog
provides:
  - All 7 Phase 4 success criteria validated on real hardware
  - 3 firmware fixes (heartbeat gate, mDNS boot ordering, 5s timeout)
  - VALIDATION.md sign-off report
  - Working Companion StreamDeck page
affects: []

tech-stack:
  added: []
  patterns:
    - "hasReceivedOsc_ gate pattern — heartbeat only active after first OSC packet"
    - "Event handler registration before WiFi.begin() — ensures GOT_IP fires on first connect"
    - "Simple press/release Companion buttons with 5s firmware safety timeout"

key-files:
  created:
    - .planning/phases/04-end-to-end-hardware-validation/VALIDATION.md
  modified:
    - src/ptz_osc.h
    - src/ptz_osc.cpp
    - src/main.cpp
    - src/ptz_config.h
    - src/ptz_wifi.cpp
    - .planning/phases/04-end-to-end-hardware-validation/COMPANION-SETUP.md

key-decisions:
  - "Heartbeat watchdog gated by hasReceivedOsc_ — serial debugging works without interference"
  - "Heartbeat timeout 500ms → 5000ms — enables simple press/release Companion buttons"
  - "mDNS event handlers registered before WiFi.begin() — fixes first-boot mDNS"
  - "COMPANION-SETUP.md rewritten for v4.2.6 simple press/release (no repeat loops)"

patterns-established:
  - "hasReceivedOsc_ gate: heartbeat watchdog only applies after OSC session starts"
  - "WiFi event handlers registered before connection attempt"

requirements-completed: [PLAT-05, MOT-01, MOT-02, MOT-03, MOT-04, MOT-05, MOT-06, MOT-07, SPD-01, SPD-02, SPD-03, NET-01, NET-02, NET-03, NET-04, NET-05, FB-01, FB-02, FB-03, FB-04]

duration: ~45min
completed: 2026-04-09
---

# Phase 4 Plan 02: Hardware Validation Summary

**All 7 success criteria pass on real ESP32+stepper hardware with 3 firmware fixes for heartbeat gating, mDNS boot ordering, and Companion-friendly timeout**

## Performance

- **Duration:** ~45 min (interactive hardware testing with user)
- **Started:** 2026-04-09
- **Completed:** 2026-04-09
- **Tasks:** 3 (2 checkpoint, 1 auto)
- **Files modified:** 6

## Accomplishments
- All 3 motor axes verified on real hardware (pan/tilt/zoom, both directions, speed scaling, deceleration)
- OSC control from Companion StreamDeck confirmed with instant latency
- Feedback (RSSI, moving state, preset) arriving at Companion as integers
- mDNS `ptzhead.local` resolving after fix
- WiFi resilience confirmed — reconnects after disconnect, heartbeat auto-stops motors
- 12-button StreamDeck page operational (6 motion, 3 speed presets, 1 stop, 2 display skipped)

## Task Commits

1. **Task 1: Flash, boot, serial motor test (SC-1, SC-2)** — `e916463` (wip: heartbeat fix)
2. **Task 2: OSC, feedback, mDNS, WiFi resilience (SC-3–SC-7)** — `3616069` (fix: heartbeat gate, mDNS boot, 5s timeout)
3. **Task 3: VALIDATION.md sign-off report** — (this commit)

## Files Created/Modified
- `src/ptz_osc.h` — Added `hasReceivedOsc_` flag and accessor
- `src/ptz_osc.cpp` — Set flag on first valid OSC packet, removed boot seed
- `src/main.cpp` — Gated heartbeat watchdog with `hasReceivedOsc()` check
- `src/ptz_config.h` — Heartbeat timeout 500ms → 5000ms
- `src/ptz_wifi.cpp` — Moved event handler registration before WiFi.begin()
- `COMPANION-SETUP.md` — Rewritten for simple press/release pattern
- `VALIDATION.md` — Full pass/fail report with requirement coverage

## Decisions Made
- Heartbeat gated by `hasReceivedOsc_` rather than removed — preserves OSC safety while enabling serial debugging
- Timeout increased to 5s rather than implementing Companion repeat loops — much simpler UX
- mDNS fix via handler reordering rather than explicit `MDNS.begin()` call — cleaner, handles reconnects too
- Skipped RSSI and Preset display buttons — generic-osc module limitation, not firmware issue

## Deviations from Plan

### Auto-fixed Issues

**1. Heartbeat kills serial commands**
- **Found during:** Task 1 (serial motor test)
- **Issue:** Heartbeat watchdog stopped serial-initiated motion after 500ms
- **Fix:** Added `hasReceivedOsc_` gate; watchdog only active after first OSC packet
- **Files:** src/ptz_osc.h, src/ptz_osc.cpp, src/main.cpp

**2. mDNS not advertising on first boot**
- **Found during:** Task 2 (mDNS test)
- **Issue:** `registerWifiEventHandlers()` called after WiFi already connected; GOT_IP event missed
- **Fix:** Moved handler registration before `WiFi.begin()`
- **File:** src/ptz_wifi.cpp

**3. Companion repeat too complex**
- **Found during:** Task 2 (Companion setup)
- **Issue:** Companion v4.2.6 has no simple repeat interval; While Loop requires local variables
- **Fix:** Increased firmware heartbeat to 5s, enabling simple press/release buttons
- **File:** src/ptz_config.h, COMPANION-SETUP.md rewritten

---

**Total deviations:** 3 auto-fixed
**Impact on plan:** All fixes necessary for real-world usability. No scope creep.

## Issues Encountered
- Companion generic-osc connection went to error state on button press — resolved by deleting and re-creating connection
- Python serial library (pyserial) flaky with DTR/RTS reset on this USB-serial adapter — used manual reset or let user use pio device monitor
- `No route to host` UDP error was transient ARP cache issue — resolved after ping refreshed the route

## User Setup Required
None — hardware is configured and operational.

## Next Phase Readiness
- **Phase 4 is the final phase of milestone v1.0**
- All v1 requirements validated on hardware
- Device is bench-ready
- Acceleration values may need tuning once camera is mounted (user aware, constants clearly documented in ptz_config.h)

---
*Phase: 04-end-to-end-hardware-validation*
*Completed: 2026-04-09*
