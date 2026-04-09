# Milestones

## v1.0 OSC Overhaul (Shipped: 2026-04-09)

**Phases completed:** 4 phases, 10 plans
**LOC:** 866 lines C++ | **Commits:** 88 | **Timeline:** 91 days

**Delivered:** Complete replacement of Bluepad32 gamepad + WebSocket architecture with pure OSC control from Bitfocus Companion, validated on real hardware.

**Key accomplishments:**

1. Stripped Bluepad32/WebSocket/ownership legacy, migrated to standard espressif32 Arduino framework
2. FastAccelStepper-based 3-axis motion control with smooth acceleration/deceleration
3. OSC command dispatch (CNMAT library) with 5-second heartbeat watchdog safety net
4. 3-tier speed presets (slow 25% / medium 60% / fast 100%) switchable via OSC
5. Live feedback to Companion: per-axis moving state, active preset, WiFi RSSI — all as integers
6. mDNS discovery (`ptzhead.local` with `_osc._udp` service) + WiFi auto-reconnect
7. All 24 v1 requirements validated on real ESP32 + stepper hardware with VALIDATION.md sign-off

**Phases:**
- Phase 1: Platform Migration and Cleanup (2 plans)
- Phase 2: Network and Core OSC Control (3 plans)
- Phase 3: Feedback and Discovery (3 plans)
- Phase 4: End-to-End Hardware Validation (2 plans)

---
