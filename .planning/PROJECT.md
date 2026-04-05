# PTZHead v2 — OSC Overhaul

## What This Is

ESP32-based Pan-Tilt-Zoom camera head controller that receives OSC commands over WiFi. Designed to be controlled from a StreamDeck via Bitfocus Companion. Drives three stepper motors (pan, tilt, zoom) with smooth acceleration. Replaces the previous gamepad + WebSocket architecture with a pure OSC interface.

## Core Value

Reliable, low-latency OSC control of a 3-axis PTZ head from Bitfocus Companion — hold a button, head moves, release and it stops smoothly.

## Requirements

### Validated

- ✓ 3-axis stepper motor control (pan, tilt, zoom) via AccelStepper — existing
- ✓ WiFi provisioning via captive portal (WiFiManager) — existing
- ✓ Smooth acceleration/deceleration on all axes — existing
- ✓ Idle motor driver shutdown — existing
- ✓ Rate-limited serial logging — existing
- ✓ OSC command input for velocity control (hold-to-move with accel/decel) — Validated in Phase 2
- ✓ OSC command input for stop (all axes or per-axis) — Validated in Phase 2
- ✓ Speed/acceleration presets switchable via OSC — Validated in Phase 2
- ✓ OSC status output (moving state, preset, RSSI) — Validated in Phase 3
- ✓ mDNS advertisement (ptzhead.local) for network discovery — Validated in Phase 3

### Active

- [ ] Fixed IP fallback for manual Companion configuration
- [ ] Remove Bluepad32 gamepad support entirely
- [ ] Remove WebSocket API entirely
- [ ] Clean up ownership system (single-source: OSC only)

### Out of Scope

- Position presets (save/recall) — no encoders yet, defer until hardware supports it
- Gamepad/Bluetooth input — replaced by OSC
- WebSocket API — replaced by OSC
- Web UI dashboard — not needed with Companion as control surface
- Multi-client arbitration — single OSC source assumed for now

## Context

- **Existing codebase:** Working firmware with AccelStepper motion, WiFiManager provisioning, and serial logging. These modules are solid and should be preserved/refactored.
- **Modules to remove:** `ptz_gamepad` (Bluepad32), `ptz_ws` (WebSocket), `ptz_owner` (ownership arbitration). The ownership system is overkill when there's only one input source.
- **Control surface:** Bitfocus Companion on a StreamDeck sends OSC messages. Buttons are momentary (hold = move, release = stop). Companion can also receive OSC for feedback (button LEDs, status text).
- **OSC library:** Need to select an ESP32-compatible OSC library (e.g. CNMAT/OSC or MicroOsc).
- **Network:** WiFi only, no Ethernet. mDNS for zero-config discovery. WiFiManager captive portal for initial setup.
- **Hardware unchanged:** Same ESP32 dev board, same 3 stepper drivers, same pin assignments.

## Constraints

- **Platform:** ESP32 dev board with PlatformIO, Arduino framework
- **Framework:** Must drop Bluepad32-patched Arduino core — switch to standard espressif32 Arduino framework since Bluepad32 is no longer needed
- **Protocol:** OSC over UDP (standard for Companion integration)
- **Latency:** Sub-50ms response to OSC commands for responsive feel
- **Memory:** ESP32 RAM constraints — avoid heap-heavy patterns

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Drop Bluepad32 + gamepad | Replaced by StreamDeck/Companion OSC control | — Pending |
| Drop WebSocket, go pure OSC | Simpler protocol, native Companion support, bidirectional | — Pending |
| Keep WiFiManager captive portal | Zero-config setup, already working, no recompile needed | — Pending |
| Switch to standard Arduino ESP32 framework | Bluepad32-patched core no longer needed | — Pending |
| Remove ownership arbitration | Single OSC source, no multi-client needed | — Pending |
| Keep AccelStepper motion module | Proven, working, good acceleration handling | — Pending |
| mDNS + fixed IP for discovery | Zero-config with manual fallback | — Pending |

---
*Last updated: 2026-04-05 after Phase 3 (feedback-and-discovery) complete — OSC status feedback + mDNS discovery delivered*
