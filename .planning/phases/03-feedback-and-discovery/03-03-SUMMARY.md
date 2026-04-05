---
phase: 03-feedback-and-discovery
plan: 03
subsystem: infra
tags: [mdns, espmdns, bonjour, discovery, wifi, esp32, osc]

# Dependency graph
requires:
  - phase: 03-feedback-and-discovery
    provides: mDNS constants (kMdnsHostname, kMdnsInstanceName, kMdnsServiceType, kMdnsServiceProto) and kLogRateMdns from Plan 01
  - phase: 02-network-and-core-osc-control
    provides: WiFi event handler scaffolding (GOT_IP, DISCONNECTED) and kOscPort=8000
provides:
  - Zero-config discovery of ptzhead.local via mDNS
  - _osc._udp:8000 Bonjour service advertisement with PTZHead instance name
  - Restart-on-GOT_IP pattern mitigating ESP32 mDNS 2-minute silence bug
affects: [04-end-to-end-hardware-validation, companion-integration]

# Tech tracking
tech-stack:
  added: [ESPmDNS (bundled with arduino-esp32 core, no lib_deps change)]
  patterns: [mDNS lifecycle tied to WiFi GOT_IP event, end-then-begin idempotent restart]

key-files:
  created: []
  modified:
    - src/ptz_wifi.cpp

key-decisions:
  - "mDNS lifecycle bound to GOT_IP event (not setup()) because IP-bound interface is required"
  - "MDNS.end() called before every MDNS.begin() to recover from stuck responder state on reconnect"
  - "Reused existing kLogRateMdns (slot 8) with 5s rate-limit to prevent log spam on fast reconnect cycles"

patterns-established:
  - "Pattern: Network-service lifecycle (mDNS, etc.) attached to WiFi GOT_IP event for reconnect resilience"
  - "Pattern: end() before begin() on network responders — idempotent on first call, curative on reconnects"

requirements-completed: [NET-03]

# Metrics
duration: 1min
completed: 2026-04-05
---

# Phase 03 Plan 03: mDNS Discovery Summary

**ESPmDNS advertisement of ptzhead.local with _osc._udp:8000 service, restarted on every WiFi GOT_IP event to mitigate ESP32 mDNS silence bug.**

## Performance

- **Duration:** ~1 min
- **Started:** 2026-04-05T18:50:18Z
- **Completed:** 2026-04-05T18:51:02Z
- **Tasks:** 1
- **Files modified:** 1

## Accomplishments
- Device now discoverable as `ptzhead.local` on any local network (zero-config)
- Bonjour browsers display `PTZHead` instance advertising `_osc._udp:8000`
- mDNS responder automatically restarts on WiFi reconnect (Pitfall 1 mitigation)
- No new PlatformIO dependencies — ESPmDNS comes with arduino-esp32 core

## Task Commits

1. **Task 1: Add ESPmDNS lifecycle to GOT_IP event handler** - `8c38fa9` (feat)

## Files Created/Modified
- `src/ptz_wifi.cpp` - Added `#include <ESPmDNS.h>`, extended GOT_IP handler with MDNS.end() → MDNS.begin(kMdnsHostname) → setInstanceName + addService; rate-limited log via kLogRateMdns

## Decisions Made
- Used `MDNS.end()` unconditionally before `MDNS.begin()` — safe on first call, curative on reconnects
- Placed mDNS start inside GOT_IP handler (the only point where an IP-bound interface is guaranteed)
- Rate-limited the advertisement log to 5s to prevent spam on fast reconnect cycles

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None. Firmware compiles cleanly, ESPmDNS resolved from arduino-esp32 core via LDF without any lib_deps entry (as predicted in the plan).

## User Setup Required

None - no external service configuration required. Hardware validation (Phase 4) will verify via:
- `ping ptzhead.local` from a host on same subnet
- `dns-sd -B _osc._udp local.` listing `PTZHead` instance

## Next Phase Readiness
- NET-03 requirement satisfied; Phase 03 plans 01/02/03 all complete
- Phase 04 hardware validation can now cover mDNS resolution end-to-end alongside OSC I/O
- Known unknown: first GOT_IP on boot may fire before `registerWifiEventHandlers()` is called in `tryStoredCredentials()`; subsequent reconnect cycles will start mDNS correctly. If Phase 4 testing shows mDNS doesn't advertise at boot, a follow-up fix would call the mDNS stanza directly after handler registration.

## Self-Check: PASSED

- FOUND: src/ptz_wifi.cpp (modified, contains all required MDNS calls)
- FOUND: commit 8c38fa9

---
*Phase: 03-feedback-and-discovery*
*Completed: 2026-04-05*
