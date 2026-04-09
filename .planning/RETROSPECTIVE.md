# Project Retrospective

*A living document updated after each milestone. Lessons feed forward into future planning.*

## Milestone: v1.0 — OSC Overhaul

**Shipped:** 2026-04-09
**Phases:** 4 | **Plans:** 10 | **Timeline:** 91 days

### What Was Built
- Complete firmware architecture replacement: Bluepad32/WebSocket/ownership → pure OSC
- 3-axis stepper motor control with FastAccelStepper (ISR-driven, no polling)
- OSC command dispatch with CNMAT library, heartbeat watchdog safety net
- 3-tier speed presets switchable via OSC
- Live feedback (moving state, preset, RSSI) as integers back to Companion
- mDNS discovery + WiFi auto-reconnect
- 12-button StreamDeck page for full PTZ control

### What Worked
- **Phase-by-phase building:** Each phase produced compilable firmware — never had a broken build
- **Deferred hardware testing to Phase 4:** Allowed pure software development in Phases 1-3 without needing hardware connected, then comprehensive testing at the end
- **Research phase before planning:** Caught pitfalls early (mDNS 2-minute bug, CNMAT heap concerns, enable pin polarity)
- **Interactive hardware testing:** Having Claude flash, read serial logs, and send commands via Python made testing efficient despite not having direct hardware access
- **Simple over complex:** Increasing heartbeat timeout to 5s was much better than implementing Companion repeat loops

### What Was Inefficient
- **mDNS event handler ordering bug:** Should have been caught during Phase 3 code review — registering handlers after WiFi.begin() is a known ESP32 pattern
- **Companion repeat loop rabbit hole:** Spent time researching While Loop + local variables in Companion v4.2.6 before realizing the firmware-side fix (longer timeout) was simpler
- **Python serial flakiness:** pyserial + DTR/RTS reset was unreliable with this USB-serial adapter — better to let user run pio device monitor directly for interactive testing
- **Phase 4 Plan 02 non-autonomous:** Interactive hardware testing required many back-and-forth exchanges; future hardware validation phases should have more explicit "user does X, reports Y" structure

### Patterns Established
- **hasReceivedOsc_ gate pattern:** Heartbeat watchdog only active after first OSC packet — preserves serial debugging
- **Event handlers before connection:** Register WiFi event handlers before WiFi.begin() on ESP32
- **Simple press/release buttons:** Firmware-side timeout (5s) instead of client-side repeat loops
- **On-change diff + periodic snapshot:** Feedback that's both immediate and self-healing
- **Reply-to-sender:** Use remoteIP/remotePort from received packet for feedback — zero config

### Key Lessons
1. **Test the full stack early:** The mDNS bug and heartbeat-vs-serial issue were only found on real hardware — earlier testing would have caught them sooner
2. **Know your control surface limitations:** Companion v4.2.6's generic-osc module has significant limitations (no per-path variables, complex repeat setup) — design firmware to compensate
3. **Firmware-side solutions beat client-side workarounds:** When the control surface can't do something easily, move the complexity to firmware where you have full control
4. **ESP32 WiFi event ordering matters:** GOT_IP fires during WiFi.begin(), not after — any handler that needs GOT_IP must be registered before the connection attempt

### Cost Observations
- Model mix: ~70% opus (execution), ~30% sonnet (research, planning)
- Sessions: ~5 across 91 days (not continuous development)
- Notable: Phase 4 was the most token-intensive due to interactive hardware testing

---

## Cross-Milestone Trends

### Process Evolution

| Milestone | Timeline | Phases | Key Change |
|-----------|----------|--------|------------|
| v1.0 | 91 days | 4 | Initial milestone — established phase-based workflow with deferred hardware testing |

### Cumulative Quality

| Milestone | Requirements | Coverage | Hardware Validated |
|-----------|-------------|----------|-------------------|
| v1.0 | 24/24 | 100% | Yes — all axes, OSC, feedback, mDNS, WiFi resilience |
