# Roadmap: PTZHead v2 — OSC Overhaul

## Overview

This roadmap replaces the Bluepad32 gamepad + WebSocket architecture with pure OSC control from Bitfocus Companion. Phase 1 cleans house — removes legacy modules and migrates to the standard ESP32 framework. Phase 2 delivers the core value: WiFi-hardened, OSC-driven 3-axis motion with speed presets. Phase 3 closes the loop with status feedback to Companion and mDNS discovery. Every phase produces firmware that compiles, boots, and can be verified on hardware.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [ ] **Phase 1: Platform Migration and Cleanup** - Remove legacy modules, switch to standard espressif32 framework, validate baseline
- [ ] **Phase 2: Network and Core OSC Control** - WiFi hardening, OSC command input, motor dispatch, speed presets
- [ ] **Phase 3: Feedback and Discovery** - OSC status feedback to Companion, heartbeat watchdog, mDNS advertisement

## Phase Details

### Phase 1: Platform Migration and Cleanup
**Goal**: Firmware compiles and boots on the standard espressif32 Arduino framework with all legacy input modules removed
**Depends on**: Nothing (first phase)
**Requirements**: PLAT-01, PLAT-02, PLAT-03, PLAT-04, PLAT-05
**Success Criteria** (what must be TRUE):
  1. Firmware compiles against standard espressif32 platform (no Bluepad32 patched core) and uploads to the ESP32
  2. Firmware boots, connects to WiFi via WiFiManager, and motors can be driven from the main loop (existing AccelStepper test)
  3. No references to Bluepad32, WebSocket, or ownership modules remain in the source tree
  4. Main loop is simplified to a single-source structure ready for OSC input (no gamepad/WebSocket dispatch logic)
**Plans:** 2 plans

Plans:
- [ ] 01-01-PLAN.md — Strip legacy modules, migrate platformio.ini to standard espressif32, clean config
- [ ] 01-02-PLAN.md — Rewrite ptz_motion for FastAccelStepper, add serial test commands, hardware verify

### Phase 2: Network and Core OSC Control
**Goal**: A user holding a Companion button drives the PTZ head via OSC over WiFi with smooth acceleration and configurable speed presets
**Depends on**: Phase 1
**Requirements**: NET-01, NET-02, NET-03, NET-04, NET-05, MOT-01, MOT-02, MOT-03, MOT-04, MOT-05, MOT-06, MOT-07, SPD-01, SPD-02, SPD-03
**Success Criteria** (what must be TRUE):
  1. Sending `/ptz/pan`, `/ptz/tilt`, or `/ptz/zoom` OSC messages from any OSC sender causes the corresponding motor to move at the commanded velocity with smooth acceleration
  2. Sending `/ptz/stop` stops all axes smoothly; per-axis stop commands (`/ptz/pan/stop`, etc.) stop individual axes
  3. Sending `/ptz/speed/preset` switches the active speed/acceleration profile and the change is immediately observable in motor behavior
  4. WiFi reconnects automatically after a network drop without requiring a power cycle, and motors auto-stop when no OSC command is received within the heartbeat timeout
  5. WiFi power save is disabled and OSC command-to-motor-response latency is under 50ms
**Plans**: TBD

Plans:
- [ ] 02-01: TBD

### Phase 3: Feedback and Discovery
**Goal**: Companion receives live status from the PTZ head (moving state, speed preset, signal strength) and the device is discoverable via mDNS
**Depends on**: Phase 2
**Requirements**: FB-01, FB-02, FB-03, FB-04
**Success Criteria** (what must be TRUE):
  1. Companion receives per-axis moving state as integer values (0/1) and can light button LEDs based on whether an axis is currently moving
  2. Companion receives the active speed preset ID and WiFi RSSI as integer values for display in button text or variables
  3. All feedback values are integers (no floats) compatible with Companion generic-osc module
  4. Device advertises as `ptzhead.local` with `_osc._udp` service type and is resolvable from the local network
**Plans**: TBD

Plans:
- [ ] 03-01: TBD

## Progress

**Execution Order:**
Phases execute in numeric order: 1 → 2 → 3

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Platform Migration and Cleanup | 0/2 | Not started | - |
| 2. Network and Core OSC Control | 0/? | Not started | - |
| 3. Feedback and Discovery | 0/? | Not started | - |
