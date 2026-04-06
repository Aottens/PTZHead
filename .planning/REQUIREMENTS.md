# Requirements: PTZHead v2

**Defined:** 2026-04-03
**Core Value:** Reliable, low-latency OSC control of a 3-axis PTZ head from Bitfocus Companion

## v1 Requirements

### Platform

- [x] **PLAT-01**: Firmware builds on standard espressif32 Arduino framework (no Bluepad32 patched core)
- [x] **PLAT-02**: Bluepad32 gamepad module is fully removed
- [x] **PLAT-03**: WebSocket API module is fully removed
- [x] **PLAT-04**: Ownership arbitration module is fully removed
- [x] **PLAT-05**: Main loop is simplified for single-source OSC input

### Motion Control

- [x] **MOT-01**: User can control pan axis velocity via OSC (`/ptz/pan`)
- [x] **MOT-02**: User can control tilt axis velocity via OSC (`/ptz/tilt`)
- [x] **MOT-03**: User can control zoom axis velocity via OSC (`/ptz/zoom`)
- [x] **MOT-04**: User can stop a single axis via OSC (`/ptz/pan/stop`, etc.)
- [x] **MOT-05**: User can stop all axes via OSC (`/ptz/stop`)
- [x] **MOT-06**: Motors accelerate and decelerate smoothly (existing AccelStepper behavior)
- [x] **MOT-07**: Firmware auto-stops all axes if no OSC command received within heartbeat timeout (watchdog)

### Speed Presets

- [x] **SPD-01**: User can switch between at least 3 speed/acceleration presets via OSC
- [x] **SPD-02**: Speed presets affect max velocity and acceleration for all axes
- [x] **SPD-03**: Active speed preset persists until changed (not per-command)

### Network

- [x] **NET-01**: Firmware connects to WiFi using stored credentials via WiFiManager captive portal
- [x] **NET-02**: WiFi power save is disabled for sub-50ms OSC response latency
- [x] **NET-03**: Firmware advertises via mDNS as `ptzhead.local` with `_osc._udp` service
- [x] **NET-04**: Firmware listens for OSC on a configurable UDP port
- [x] **NET-05**: Firmware reconnects to WiFi automatically on disconnect (non-blocking, event-based)

### OSC Feedback

- [x] **FB-01**: Firmware sends per-axis moving state back via OSC (integer: 0/1)
- [x] **FB-02**: Firmware sends active speed preset ID back via OSC (integer)
- [x] **FB-03**: Firmware sends WiFi RSSI back via OSC (integer)
- [x] **FB-04**: All OSC feedback uses integer values (Companion generic-osc compatibility)

## v2 Requirements

### Position Presets

- **POS-01**: User can save current position to a preset slot
- **POS-02**: User can recall a saved preset position
- **POS-03**: Presets persist across reboots (NVS storage)

### Configuration

- **CFG-01**: User can invert axis direction via OSC
- **CFG-02**: User can configure axis limits via OSC
- **CFG-03**: Configuration persists across reboots (NVS storage)

### Feedback

- **FB-05**: Firmware sends current step positions per axis via OSC

## Out of Scope

| Feature | Reason |
|---------|--------|
| Gamepad / Bluetooth input | Replaced by OSC from Companion |
| WebSocket API | Replaced by pure OSC |
| Multi-client arbitration | Single OSC source assumed |
| Web UI dashboard | Companion is the control surface |
| Position presets | No encoders yet — deferred to v2 |
| Homing / limit switches | No hardware support |
| Ethernet connectivity | WiFi only for this hardware |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| PLAT-01 | Phase 1 | Complete |
| PLAT-02 | Phase 1 | Complete |
| PLAT-03 | Phase 1 | Complete |
| PLAT-04 | Phase 1 | Complete |
| PLAT-05 | Phase 1 | Complete |
| MOT-01 | Phase 2 | Complete |
| MOT-02 | Phase 2 | Complete |
| MOT-03 | Phase 2 | Complete |
| MOT-04 | Phase 2 | Complete |
| MOT-05 | Phase 2 | Complete |
| MOT-06 | Phase 2 | Complete |
| MOT-07 | Phase 2 | Complete |
| SPD-01 | Phase 2 | Complete |
| SPD-02 | Phase 2 | Complete |
| SPD-03 | Phase 2 | Complete |
| NET-01 | Phase 2 | Complete |
| NET-02 | Phase 2 | Complete |
| NET-03 | Phase 3 | Complete |
| NET-04 | Phase 2 | Complete |
| NET-05 | Phase 2 | Complete |
| FB-01 | Phase 3 | Complete |
| FB-02 | Phase 3 | Complete |
| FB-03 | Phase 3 | Complete |
| FB-04 | Phase 3 | Complete |

**Coverage:**
- v1 requirements: 24 total
- Mapped to phases: 24
- Unmapped: 0

---
*Requirements defined: 2026-04-03*
*Last updated: 2026-04-03 after roadmap creation*
