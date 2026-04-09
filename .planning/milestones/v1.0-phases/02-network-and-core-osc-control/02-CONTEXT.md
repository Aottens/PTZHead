# Phase 2: Network and Core OSC Control - Context

**Gathered:** 2026-04-05
**Status:** Ready for planning

<domain>
## Phase Boundary

WiFi-hardened, OSC-driven 3-axis motion. A user holding a Companion button sends OSC over WiFi to drive pan/tilt/zoom with smooth acceleration, and switchable speed presets affect motor behavior immediately. WiFi reconnects automatically on drop; motors auto-stop when OSC traffic goes silent. OSC status feedback and mDNS discovery are out of scope (Phase 3). Receive-only OSC path in this phase.

</domain>

<decisions>
## Implementation Decisions

### OSC namespace & arguments
- Per-axis velocity addresses with float arg in -1.0..1.0 range: `/ptz/pan f`, `/ptz/tilt f`, `/ptz/zoom f`
- Per-axis only — no combined `/ptz/move` endpoint. Diagonals achieved by sending two per-axis messages.
- Stop commands are address-only, no arguments: `/ptz/stop`, `/ptz/pan/stop`, `/ptz/tilt/stop`, `/ptz/zoom/stop`
- Speed preset selector is an integer index: `/ptz/speed/preset i` where i ∈ 0..N-1
- Unknown OSC addresses are silently dropped but logged at Debug level with a rate limiter (new LogRateId)

### Speed presets
- Ship 3 presets for Phase 2: slow / medium / fast (meets SPD-01 minimum)
- Preset scales BOTH max velocity AND acceleration together (SPD-02) — slow also eases accel for smooth low-speed framing
- Starting values as multipliers of Phase 1 `kPanMaxSps = 4000`: slow=25% (1000 sps), medium=60% (2400 sps), fast=100% (4000 sps). Accel scales with same ratios from `kPanAccel`/`kTiltAccel`/`kZoomAccel`.
- Active preset at boot: medium. No NVS persistence — operator sets per session.
- Active preset persists until explicitly changed via OSC (SPD-03)

### Heartbeat & safety
- Heartbeat watchdog timeout: 500ms — if no OSC command received in 500ms, auto-stop all axes (MOT-07). Companion must send at ≥10Hz while button held.
- Auto-stop style on timeout: smooth deceleration via FastAccelStepper's stopMove() using current preset's accel value. Same stop semantics as `/ptz/stop`.
- WiFi reconnect (NET-05): event-handler driven — register WiFi.onEvent() for SYSTEM_EVENT_STA_DISCONNECTED, call WiFi.reconnect() non-blocking on disconnect. No exponential backoff in v1.
- On WiFi drop: do NOT stop motors immediately — let the 500ms heartbeat watchdog be the single stop mechanism. One stop path keeps code simple.

### OSC port & library
- OSC library: CNMAT/OSC (research recommendation). Heap behavior must be monitored — log `ESP.getFreeHeap()` trend; flag if heap trends downward over 30-minute session.
- UDP port: fixed at 8000 (OSC default, Companion generic-osc default). Declared as `constexpr` in `ptz_config.h` — no captive-portal configurability in Phase 2.
- WiFi power save must be disabled (NET-02): call `WiFi.setSleep(false)` and `esp_wifi_set_ps(WIFI_PS_NONE)` immediately after WiFi connects. Non-negotiable for <50ms latency.
- Receive-only in Phase 2 — do NOT cache sender IP/port yet. Phase 3 adds reply-to-sender for feedback.

### Claude's Discretion
- CNMAT/OSC integration details (dispatch mechanism, callback wiring vs imperative parsing)
- Static vs heap buffer sizing for UDP receive
- Heartbeat timer implementation (millis() comparison vs timer object)
- Module split — new `ptz_osc` module vs integrating into main.cpp
- Serial command parity with OSC (keep PAN/TILT/ZOOM serial commands alongside OSC, or retire them)
- Exact log rate IDs added for OSC-relevant events

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Research and architecture
- `.planning/research/SUMMARY.md` — Stack recommendation (CNMAT/OSC), build order, risk mitigations
- `.planning/research/ARCHITECTURE.md` — Module boundaries, `ptz_osc` API sketch, main loop structure
- `.planning/research/PITFALLS.md` — WiFi power save, AccelStepper/FastAccelStepper starvation, UDP packet loss watchdog, CNMAT/OSC heap behavior
- `.planning/research/STACK.md` — Library version pins (CNMAT/OSC v3.5.8, espressif32 6.13.0)

### Requirements
- `.planning/REQUIREMENTS.md` — NET-01, NET-02, NET-04, NET-05; MOT-01..MOT-07; SPD-01..SPD-03 define Phase 2 scope

### Existing code to modify/extend
- `src/main.cpp` — Current serial-driven loop; add OSC receive path and heartbeat watchdog
- `src/ptz_motion.h` / `src/ptz_motion.cpp` — Public API (setVelocity, stop) already matches OSC needs; add `applySpeedPreset(int)` method
- `src/ptz_wifi.h` / `src/ptz_wifi.cpp` — Add event-based reconnect and `WiFi.setSleep(false)` post-connect
- `src/ptz_config.h` — Add OSC port constant, speed preset struct/table, heartbeat timeout constant, new LogRateIds

### Prior phase
- `.planning/phases/01-platform-migration-and-cleanup/01-CONTEXT.md` — FastAccelStepper baseline, normalized velocity API, auto-enable behavior

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `ptz::PtzMotion`: Already exposes `setVelocity(pan, tilt, zoom)`, `stop()`, `isMoving()` — OSC handlers call these directly
- `ptz::PtzWifi`: WiFiManager provisioning intact from Phase 1 — extend with `onEvent` reconnect handler and setSleep(false)
- `ptz::log` / `PTZ_LOGx` macros: Rate-limited logging — add new LogRateIds for OSC receive, heartbeat fires, WiFi reconnect
- Serial command handler in `main.cpp`: Can stay as a debug/bring-up tool alongside OSC (Claude discretion)

### Established Patterns
- `ptz::` namespace, `constexpr` constants in `ptz_config.h`, classes own their state (engine_, steppers)
- Enable pins active-low, auto-managed by FastAccelStepper (Phase 1 decision)
- Normalized -1.0..1.0 velocity scaled internally to steps/sec via `kPanMaxSps`/`kTiltMaxSps`/`kZoomMaxSps`

### Integration Points
- `platformio.ini` lib_deps: add `cnmat/OSC@^3.5.8`
- `main.cpp` loop: add `osc.update()` (drain all UDP packets each iteration per Pitfall 8), then `motion.update()` + heartbeat check
- `ptz_motion`: extend with preset application that rewrites `maxSpeedInHz` and `acceleration` on each stepper
- New module likely: `src/ptz_osc.h` / `src/ptz_osc.cpp` — owns WiFiUDP, CNMAT dispatcher, command→motion bridge, heartbeat timer

</code_context>

<specifics>
## Specific Ideas

- Companion generic-osc module is the target sender — addresses must match the roadmap's `/ptz/*` schema so the generic-osc preset page is trivial to author.
- Buttons are momentary hold-to-move; Companion should send velocity at ≥10Hz while held, then send `/ptz/<axis>/stop` or velocity=0 on release. Redundant stops on release are acceptable.
- OSC port 8000 chosen as both OSC-standard and Companion generic-osc default — minimizes setup friction.

</specifics>

<deferred>
## Deferred Ideas

- `/ptz/move pan tilt zoom` combined endpoint — can add in a later phase if joystick-style senders emerge
- Configurable OSC port via captive portal — add in v2 if multiple devices share a network
- Axis inversion via OSC (CFG-01) — v2 per REQUIREMENTS.md
- Exponential backoff on WiFi reconnect — only add if rapid reconnect attempts cause AP issues in the field
- NVS persistence of active speed preset — operator workflow convenience, revisit if requested
- Cache sender IP for reply-to-sender — moves to Phase 3 with feedback

</deferred>

---

*Phase: 02-network-and-core-osc-control*
*Context gathered: 2026-04-05*
