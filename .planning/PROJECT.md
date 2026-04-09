# PTZHead v2 — OSC Overhaul

## What This Is

ESP32-based Pan-Tilt-Zoom camera head controller driven entirely by OSC over WiFi. Controlled from a StreamDeck via Bitfocus Companion. Drives three stepper motors (pan, tilt, zoom) with smooth acceleration using FastAccelStepper. Provides live feedback (moving state, speed preset, RSSI) back to Companion. Discoverable on the network via mDNS as `ptzhead.local`.

## Core Value

Reliable, low-latency OSC control of a 3-axis PTZ head from Bitfocus Companion — hold a button, head moves, release and it stops smoothly.

## Requirements

### Validated

- ✓ PLAT-01..05: Standard espressif32 framework, all legacy modules removed — v1.0
- ✓ MOT-01..07: 3-axis velocity control, per-axis and all-axis stop, smooth accel/decel, heartbeat watchdog — v1.0
- ✓ SPD-01..03: 3-tier speed presets (slow/medium/fast) switchable via OSC, persistent — v1.0
- ✓ NET-01..05: WiFiManager provisioning, WIFI_PS_NONE, mDNS ptzhead.local, auto-reconnect — v1.0
- ✓ FB-01..04: Per-axis moving state, preset, RSSI as integer feedback to Companion — v1.0

### Active

- [ ] POS-01: Save current position to a preset slot
- [ ] POS-02: Recall a saved preset position
- [ ] POS-03: Presets persist across reboots (NVS storage)
- [ ] CFG-01: Invert axis direction via OSC
- [ ] CFG-02: Configure axis limits via OSC
- [ ] CFG-03: Configuration persists across reboots (NVS storage)
- [ ] FB-05: Current step positions per axis via OSC

### Out of Scope

- Gamepad / Bluetooth input — replaced by OSC from Companion
- WebSocket API — replaced by pure OSC
- Multi-client arbitration — single OSC source assumed
- Web UI dashboard — Companion is the control surface
- Homing / limit switches — no hardware support
- Ethernet connectivity — WiFi only for this hardware

## Context

**Shipped v1.0** with 866 LOC C++ across 7 source files.
**Tech stack:** ESP32 (espressif32 6.10.0), Arduino framework, FastAccelStepper, CNMAT/OSC, WiFiManager, ESPmDNS.
**Firmware size:** 69.7% flash, 16.1% RAM, 241K free heap.
**Hardware validated:** All 3 axes working on real stepper hardware, OSC from Companion confirmed instant latency, WiFi resilience verified.

**Known tuning needed:**
- Acceleration values (currently 20,000 pan/tilt, 15,000 zoom) will need adjustment once camera is mounted
- Companion generic-osc module v2.8.2 doesn't support per-path feedback variables (firmware sends correctly, module limitation)
- RSSI -79 to -94 dBm — adequate but could improve with antenna placement

## Constraints

- **Platform:** ESP32 dev board with PlatformIO, Arduino framework (espressif32 6.10.0)
- **Protocol:** OSC over UDP (standard for Companion integration)
- **Latency:** Sub-50ms response to OSC commands — achieved
- **Memory:** ESP32 RAM constraints — heap stable at 241K, no leaks detected
- **Control surface:** Bitfocus Companion v4.2.6 with generic-osc module v2.8.2

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Drop Bluepad32 + gamepad | Replaced by StreamDeck/Companion OSC control | ✓ Good — cleaner architecture |
| Drop WebSocket, go pure OSC | Simpler protocol, native Companion support | ✓ Good — single protocol |
| Keep WiFiManager captive portal | Zero-config setup, already working | ✓ Good — credentials persisted across framework switch |
| Switch to standard Arduino ESP32 framework | Bluepad32-patched core no longer needed | ✓ Good — standard toolchain |
| FastAccelStepper over AccelStepper | ISR-driven, no polling needed | ✓ Good — smooth motion, low CPU |
| CNMAT/OSC library | Only mature ESP32-compatible OSC library | ✓ Good — stable, no heap leaks |
| Heartbeat watchdog gated by hasReceivedOsc_ | Serial debugging needs to work without watchdog interference | ✓ Good — both serial and OSC work correctly |
| Heartbeat timeout 5s (was 500ms) | Companion v4.2.6 can't easily do repeat sends; simple press/release pattern | ✓ Good — much simpler UX |
| mDNS restart-on-GOT_IP | ESP32 mDNS goes silent after ~2min; community fix | ✓ Good — resolves reliably |
| Event handlers before WiFi.begin() | GOT_IP fires during connection, not after | ✓ Good — mDNS works on first boot |
| On-change diff + 1Hz self-heal snapshot | Immediate feedback updates + recovery from packet loss | ✓ Good — Companion sees updates |
| Reply-to-sender feedback (no broadcast) | Uses remoteIP/remotePort from received packet | ✓ Good — works without config |

---
*Last updated: 2026-04-09 after v1.0 milestone complete — device bench-ready*
