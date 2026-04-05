# Roadmap: PTZHead v2 — OSC Overhaul

## Overview

This roadmap replaces the Bluepad32 gamepad + WebSocket architecture with pure OSC control from Bitfocus Companion. Phase 1 cleans house — removes legacy modules and migrates to the standard ESP32 framework. Phase 2 delivers the core value: WiFi-hardened, OSC-driven 3-axis motion with speed presets. Phase 3 closes the loop with status feedback to Companion and mDNS discovery. Phase 4 runs comprehensive end-to-end hardware validation of all features built in phases 1–3. Every phase produces firmware that compiles, boots, and can be verified on hardware.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [ ] **Phase 1: Platform Migration and Cleanup** - Remove legacy modules, switch to standard espressif32 framework, validate baseline
- [ ] **Phase 2: Network and Core OSC Control** - WiFi hardening, OSC command input, motor dispatch, speed presets
- [ ] **Phase 3: Feedback and Discovery** - OSC status feedback to Companion, heartbeat watchdog, mDNS advertisement
- [ ] **Phase 4: End-to-End Hardware Validation** - Flash, bench-test, and sign off on motion/OSC/feedback behavior on real hardware

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
- [x] 01-01-PLAN.md — Strip legacy modules, migrate platformio.ini to standard espressif32, clean config
- [x] 01-02-PLAN.md — Rewrite ptz_motion for FastAccelStepper, add serial test commands (hardware verify → Phase 4)

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
**Plans:** 3 plans

Plans:
- [ ] 02-01-PLAN.md — Wave 1: Add OSC/heartbeat/preset constants + LogRateIds to ptz_config.h; harden WiFi (WIFI_PS_NONE, event-driven reconnect)
- [ ] 02-02-PLAN.md — Wave 1: Extend PtzMotion with per-axis setters, per-axis stops, and applySpeedPreset(idx) scaling max-sps + acceleration
- [ ] 02-03-PLAN.md — Wave 2: Create ptz_osc module (CNMAT dispatch, WiFiUDP, drain-all-packets); wire main loop with 500ms heartbeat watchdog

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

### Phase 4: End-to-End Hardware Validation
**Goal**: Every feature built in Phases 1–3 is confirmed working on real ESP32 + stepper hardware; deferred Phase 1 checkpoint is closed and the device is declared bench-ready
**Depends on**: Phase 1, Phase 2, Phase 3
**Requirements**: PLAT-05 (hardware verification, deferred from Phase 1), plus end-to-end validation of NET/MOT/SPD/FB requirements
**Success Criteria** (what must be TRUE):
  1. Firmware flashes cleanly via `pio run --target upload` and boots to `Setup complete` with WiFi connected (or captive portal on first boot)
  2. Serial motor test commands drive all 3 axes correctly — direction matches expectation (`kInvertPan` validated), speed scales linearly with norm value, `STOP` decelerates smoothly, motors auto-disable ~500ms after stop
  3. OSC commands from Companion (or any OSC sender) produce motor motion within the 50ms latency target on WiFi
  4. Status feedback (axis moving flags, speed preset, RSSI) arrives at the OSC sender and updates in real time
  5. WiFi survives a network drop and reconnects without a power cycle; heartbeat timeout triggers auto-stop
  6. mDNS resolves `ptzhead.local` from another device on the network
  7. Any hardware-specific issues found (pin polarity, timing, thermal, noise) are captured as fixes or documented constraints
**Plans**: TBD

Plans:
- [ ] 04-01: TBD

## Progress

**Execution Order:**
Phases execute in numeric order: 1 → 2 → 3 → 4

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Platform Migration and Cleanup | 2/2 | Code-complete (HW verify deferred to Phase 4) | 2026-04-05 |
| 2. Network and Core OSC Control | 0/? | Not started | - |
| 3. Feedback and Discovery | 0/? | Not started | - |
| 4. End-to-End Hardware Validation | 0/? | Not started | - |
