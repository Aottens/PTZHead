# Phase 2: Network and Core OSC Control - Research

**Researched:** 2026-04-05
**Domain:** ESP32 OSC-over-UDP control for 3-axis stepper PTZ head (CNMAT/OSC + WiFiUDP + FastAccelStepper)
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

**OSC namespace & arguments**
- Per-axis velocity addresses with float arg in -1.0..1.0 range: `/ptz/pan f`, `/ptz/tilt f`, `/ptz/zoom f`
- Per-axis only — no combined `/ptz/move` endpoint. Diagonals achieved by sending two per-axis messages.
- Stop commands are address-only, no arguments: `/ptz/stop`, `/ptz/pan/stop`, `/ptz/tilt/stop`, `/ptz/zoom/stop`
- Speed preset selector is an integer index: `/ptz/speed/preset i` where i ∈ 0..N-1
- Unknown OSC addresses are silently dropped but logged at Debug level with a rate limiter (new LogRateId)

**Speed presets**
- Ship 3 presets for Phase 2: slow / medium / fast (meets SPD-01 minimum)
- Preset scales BOTH max velocity AND acceleration together (SPD-02) — slow also eases accel for smooth low-speed framing
- Starting values as multipliers of Phase 1 `kPanMaxSps = 4000`: slow=25% (1000 sps), medium=60% (2400 sps), fast=100% (4000 sps). Accel scales with same ratios from `kPanAccel`/`kTiltAccel`/`kZoomAccel`.
- Active preset at boot: medium. No NVS persistence — operator sets per session.
- Active preset persists until explicitly changed via OSC (SPD-03)

**Heartbeat & safety**
- Heartbeat watchdog timeout: 500ms — if no OSC command received in 500ms, auto-stop all axes (MOT-07). Companion must send at ≥10Hz while button held.
- Auto-stop style on timeout: smooth deceleration via FastAccelStepper's stopMove() using current preset's accel value. Same stop semantics as `/ptz/stop`.
- WiFi reconnect (NET-05): event-handler driven — register WiFi.onEvent() for SYSTEM_EVENT_STA_DISCONNECTED, call WiFi.reconnect() non-blocking on disconnect. No exponential backoff in v1.
- On WiFi drop: do NOT stop motors immediately — let the 500ms heartbeat watchdog be the single stop mechanism. One stop path keeps code simple.

**OSC port & library**
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

### Deferred Ideas (OUT OF SCOPE)
- `/ptz/move pan tilt zoom` combined endpoint — can add in a later phase if joystick-style senders emerge
- Configurable OSC port via captive portal — add in v2 if multiple devices share a network
- Axis inversion via OSC (CFG-01) — v2 per REQUIREMENTS.md
- Exponential backoff on WiFi reconnect — only add if rapid reconnect attempts cause AP issues in the field
- NVS persistence of active speed preset — operator workflow convenience, revisit if requested
- Cache sender IP for reply-to-sender — moves to Phase 3 with feedback
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| NET-01 | WiFi connects via WiFiManager captive portal | Already implemented in `ptz_wifi.cpp` Phase 1; keep as-is, extend only with event handler + setSleep(false) |
| NET-02 | WiFi power save disabled for <50ms latency | Call `WiFi.setSleep(false)` + `esp_wifi_set_ps(WIFI_PS_NONE)` immediately after `WL_CONNECTED`; replace current `WIFI_PS_MIN_MODEM` |
| NET-03 | mDNS `ptzhead.local` + `_osc._udp` | OUT OF SCOPE — belongs to Phase 3 per ROADMAP.md |
| NET-04 | Configurable UDP port | Port 8000 as `constexpr kOscPort` in `ptz_config.h`; "configurable" satisfied at compile-time (captive-portal deferred) |
| NET-05 | Auto-reconnect, non-blocking, event-based | `WiFi.onEvent(…, ARDUINO_EVENT_WIFI_STA_DISCONNECTED)`; call `WiFi.reconnect()` from handler (no blocking, no backoff) |
| MOT-01 | `/ptz/pan` velocity control | CNMAT `dispatch("/ptz/pan", cb)`; float arg → `motion.setPanVelocity(norm)` → `setSpeedInHz()` + `applySpeedAcceleration()` + `runForward/Backward()` |
| MOT-02 | `/ptz/tilt` velocity control | Same pattern as pan; add `setTiltVelocity(float)` to PtzMotion |
| MOT-03 | `/ptz/zoom` velocity control | Same pattern as pan; add `setZoomVelocity(float)` to PtzMotion |
| MOT-04 | Per-axis stop | `dispatch("/ptz/pan/stop", …)` etc. → `stepper->stopMove()` (smooth decel) |
| MOT-05 | All-stop | `dispatch("/ptz/stop", …)` → `motion.stop()` (already implemented) |
| MOT-06 | Smooth accel/decel | FastAccelStepper handles autonomously once `setAcceleration()` + `applySpeedAcceleration()` called |
| MOT-07 | Heartbeat watchdog auto-stop | Track `lastOscRxMs`; if `millis() - lastOscRxMs > 500` AND motion active, call `motion.stop()` (smooth decel via stopMove) |
| SPD-01 | ≥3 speed presets switchable via OSC | `constexpr SpeedPreset kSpeedPresets[3]`; `/ptz/speed/preset i` handler |
| SPD-02 | Presets affect max velocity AND acceleration for all axes | `applySpeedPreset(idx)` rewrites per-axis `maxSpeedInHz` scaling factor + calls `setAcceleration()` on all 3 steppers; followed by `applySpeedAcceleration()` if running |
| SPD-03 | Active preset persists until changed | Store `g_activePreset` as module-level state; no NVS in Phase 2 |
</phase_requirements>

## Summary

Phase 2 layers OSC-over-UDP command reception on top of the FastAccelStepper-driven motion module already built in Phase 1. The work is narrowly scoped: add one library (`CNMAT/OSC@^3.5.8`), create one new module (`ptz_osc`), extend two existing modules (`ptz_motion` with per-axis setters + `applySpeedPreset`; `ptz_wifi` with an event-driven reconnect handler and `setSleep(false)`), and thread a 500ms heartbeat watchdog through the main loop.

The critical technical risks are well-understood and have clear mitigations: WiFi power save MUST be disabled before any latency measurement is meaningful (NET-02); CNMAT/OSC's internal heap allocation behavior must be observed (not pre-optimized) via `ESP.getFreeHeap()` logging; and the UDP receive path must drain ALL queued packets per loop iteration to avoid packet bunching delivering stale velocity commands. FastAccelStepper's existing ISR-driven stepping insulates the motor hot path from OSC parsing overhead — this was the main reliability win from Phase 1.

The locked decisions in CONTEXT.md resolve every remaining design question except small integration tactics: whether to use CNMAT's `msg.dispatch()` per-address vs a single `bundleIN.route()` tree, whether to keep serial PAN/TILT/ZOOM commands as a debug path, and exact LogRateId layout. The planner has discretion on those.

**Primary recommendation:** Create `src/ptz_osc.{h,cpp}` owning `WiFiUDP` + a static 128-byte receive buffer + the heartbeat timer; use CNMAT `OSCMessage::dispatch()` with exact-address matching (not `route()` prefix matching, which would make `/ptz/pan` collide with `/ptz/pan/stop`); drain all packets per loop iteration; extend `PtzMotion` with three per-axis setters and `applySpeedPreset(uint8_t idx)`; register WiFi STA_DISCONNECTED event handler in `PtzWifi::begin()` right after successful connect; call `WiFi.setSleep(false)` and `esp_wifi_set_ps(WIFI_PS_NONE)` in the same post-connect block.

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| CNMAT/OSC | `^3.5.8` | OSC message encoding/decoding, address dispatch | De facto Arduino OSC library (795★), full OSC 1.0 spec, ESP32 WiFiUDP examples shipped in repo, battle-tested since 2013, verified ESP32-compatible since 2018 (issue #59 resolved) |
| WiFiUDP | bundled (arduino-esp32 2.0.17) | UDP socket transport | Ships with Arduino core; OSC messages for this project are ~16-32 bytes, single-packet, zero fragmentation risk |
| FastAccelStepper | `^0.31.0` | Stepper motor control (ISR-driven) | Already in `platformio.ini` from Phase 1; hardware-timed stepping decouples motor timing from OSC parsing jitter |
| WiFiManager (tzapu) | `^2.0.17` | WiFi provisioning | Already in `platformio.ini`; keep as-is, only add event-handler reconnect |
| esp_wifi.h (ESP-IDF) | bundled | `esp_wifi_set_ps(WIFI_PS_NONE)` | Required alongside Arduino `WiFi.setSleep(false)` for guaranteed power-save disable |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| (none) | — | — | No additional libraries needed. All infrastructure bundles with arduino-esp32 core. |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| CNMAT/OSC | MicroOsc | Zero-allocation (no heap fragmentation), but less mature, minimal error checking, smaller community. Research SUMMARY.md flags CNMAT heap behavior as a risk — if `ESP.getFreeHeap()` trends down over 30-min session, MicroOsc is the documented fallback. NOT switching pre-emptively. |
| CNMAT/OSC | ArduinoOSC (hideakitai) | Over-engineered template API, harder to debug on embedded. Rejected. |

**Installation (append to existing `lib_deps`):**
```ini
lib_deps =
  tzapu/WiFiManager@^2.0.17
  gin66/FastAccelStepper@^0.31.0
  cnmat/OSC@^3.5.8
```

**Version verification:**
- CNMAT/OSC `3.5.8` verified 2026-04-05 against [library.json on master](https://github.com/CNMAT/OSC/blob/master/library.json) — released Sep 4, 2023, still current.
- espressif32 platform `6.10.0` (current in platformio.ini) is acceptable. Bumping to `6.13.0` is OPTIONAL and out-of-scope for Phase 2 (Phase 1 did not bump). Do NOT touch platform version in Phase 2.
- FastAccelStepper `^0.31.0` already pinned in Phase 1 — leave alone.

## Architecture Patterns

### Recommended Project Structure
```
src/
├── main.cpp          # setup() + loop(); owns heartbeat timer; serial commands retained as debug tool
├── ptz_config.h      # + kOscPort, kHeartbeatTimeoutMs, SpeedPreset struct + kSpeedPresets[3], kDefaultPresetIndex, new LogRateIds
├── ptz_motion.{h,cpp} # + setPanVelocity/setTiltVelocity/setZoomVelocity, applySpeedPreset(uint8_t)
├── ptz_osc.{h,cpp}   # NEW: owns WiFiUDP, CNMAT dispatch, points to PtzMotion, exposes lastRxMs() for heartbeat
├── ptz_wifi.{h,cpp}  # + event-handler reconnect, setSleep(false) + esp_wifi_set_ps(WIFI_PS_NONE) post-connect
└── ptz_log.{h,cpp}   # unchanged (extend LogRateId enum in ptz_config.h)
```

### Pattern 1: Exact-Address Dispatch (not prefix route)
**What:** Use `OSCMessage::dispatch("/addr", cb)` for each exact address, not `OSCBundle::route("/prefix", cb)` with offset matching.
**When to use:** Any time two addresses share a prefix where one is NOT a parent of the other (e.g., `/ptz/pan` f vs `/ptz/pan/stop`). `route()` would match both on `/ptz/pan`, leaking the float-arg handler into the no-arg stop.
**Example:**
```cpp
// Source: CNMAT/OSC OSCMessage.h — dispatch() requires full address match; callback receives OSCMessage&, int addressOffset
void PtzOsc::handleMessage(OSCMessage& msg) {
  msg.dispatch("/ptz/pan",       [](OSCMessage& m) { g_osc->onAxisVel(0, m); });
  msg.dispatch("/ptz/tilt",      [](OSCMessage& m) { g_osc->onAxisVel(1, m); });
  msg.dispatch("/ptz/zoom",      [](OSCMessage& m) { g_osc->onAxisVel(2, m); });
  msg.dispatch("/ptz/stop",      [](OSCMessage& m) { g_osc->onStopAll(); });
  msg.dispatch("/ptz/pan/stop",  [](OSCMessage& m) { g_osc->onStopAxis(0); });
  msg.dispatch("/ptz/tilt/stop", [](OSCMessage& m) { g_osc->onStopAxis(1); });
  msg.dispatch("/ptz/zoom/stop", [](OSCMessage& m) { g_osc->onStopAxis(2); });
  msg.dispatch("/ptz/speed/preset", [](OSCMessage& m) { g_osc->onPreset(m); });
  // No lambda capture needed — CNMAT dispatch takes a plain function pointer (not std::function).
  // Use a file-static pointer g_osc or thunk table. See Pitfall 4 below.
}
```
**Note on callback signature:** CNMAT `dispatch()` takes `void (*)(OSCMessage&)` — a plain C function pointer. Capturing lambdas WILL NOT compile. Use a file-static `PtzOsc* s_this = nullptr;` set in `begin()` and call via trampoline functions, OR inline the dispatch logic and check `msg.fullMatch("/ptz/pan")` manually.

### Pattern 2: Drain-All-Packets Receive Loop
**What:** On every `loop()` iteration, drain EVERY queued UDP packet, not just one.
**When to use:** Always. WiFi stack can bunch packets (documented pitfall). Single-packet-per-loop allows queue growth under burst, delivering stale velocities seconds later.
**Example:**
```cpp
void PtzOsc::update() {
  int size;
  while ((size = udp_.parsePacket()) > 0) {
    OSCMessage msg;
    while (size--) msg.fill(udp_.read());
    if (!msg.hasError()) {
      lastRxMs_ = millis();  // any valid packet counts as heartbeat
      handleMessage(msg);
    } else {
      if (logShouldEmit(kLogRateOscParseError, 1000)) {
        PTZ_LOGW("OSC", "parse error: %d", msg.getError());
      }
    }
  }
}
```

### Pattern 3: Heartbeat Watchdog in Main Loop
**What:** Track `lastRxMs` inside `PtzOsc` (bumped on ANY valid packet). Main loop checks `millis() - lastRxMs > 500` and, if motion is active, calls `motion.stop()` idempotently.
**When to use:** MOT-07 safety requirement. Must fire from the main loop, not the OSC path (the OSC path doesn't run when packets stop).
**Example:**
```cpp
void loop() {
  g_osc.update();
  handleSerialCommands();

  const uint32_t now = millis();
  if (g_motion.isMoving() && (now - g_osc.lastRxMs()) > ptz::kHeartbeatTimeoutMs) {
    if (logShouldEmit(ptz::kLogRateHeartbeatFired, 2000)) {
      PTZ_LOGW("OSC", "heartbeat timeout, auto-stop");
    }
    g_motion.stop();
  }
}
```
**Critical:** `lastRxMs_` must be initialized to `millis()` in `begin()` AFTER WiFi connects, OR seeded such that the watchdog does NOT fire before any packet ever arrives. Otherwise, a freshly-booted idle device with no OSC traffic will have `isMoving()==false` permanently, which is fine — but if someone sends the first velocity command and the packet arrives 2s after boot, no spurious auto-stop should have triggered. Since `isMoving()==false` gates the stop, initial-state seeding is actually not strictly required; seeding to `millis()` in `begin()` is just belt-and-suspenders.

### Pattern 4: Speed Preset as `constexpr` Table with Runtime Apply
**What:** Presets are compile-time `constexpr SpeedPreset kSpeedPresets[3]`. Active index is runtime state in `PtzMotion` (or main.cpp). `applySpeedPreset(idx)` rewrites per-axis max speed (for use at next `setVelocity()` scaling) AND calls `setAcceleration()` + `applySpeedAcceleration()` on each running stepper.
**Example:**
```cpp
// ptz_config.h
struct SpeedPreset {
  float panScale;   // multiplier on kPanMaxSps
  float tiltScale;
  float zoomScale;
  float panAccel;   // absolute steps/sec²
  float tiltAccel;
  float zoomAccel;
};
constexpr SpeedPreset kSpeedPresets[] = {
  // slow: 25% speed, 25% accel
  { 0.25f, 0.25f, 0.25f,  5000.0f,  5000.0f, 3750.0f },
  // medium (default): 60% speed, 60% accel
  { 0.60f, 0.60f, 0.60f, 12000.0f, 12000.0f, 9000.0f },
  // fast: 100% speed, 100% accel
  { 1.00f, 1.00f, 1.00f, 20000.0f, 20000.0f, 15000.0f },
};
constexpr uint8_t kDefaultPresetIndex = 1;
constexpr uint8_t kPresetCount = sizeof(kSpeedPresets) / sizeof(kSpeedPresets[0]);
```

### Pattern 5: WiFi Event-Driven Reconnect (non-blocking)
**What:** Register a lambda-free event handler via `WiFi.onEvent()` for `ARDUINO_EVENT_WIFI_STA_DISCONNECTED`. The handler calls `WiFi.reconnect()` immediately and returns. No sleep, no loop, no state.
**Example:**
```cpp
// In PtzWifi::begin(), after WiFi.status() == WL_CONNECTED:
WiFi.setSleep(false);                   // Arduino wrapper
esp_wifi_set_ps(WIFI_PS_NONE);          // IDF-level belt-and-suspenders
WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
  PTZ_LOGW("WIFI", "STA disconnected reason=%u, reconnecting", info.wifi_sta_disconnected.reason);
  WiFi.reconnect();
}, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
  PTZ_LOGI("WIFI", "GOT_IP %s", WiFi.localIP().toString().c_str());
  // Re-apply power save = off after reconnect — some reports indicate it resets.
  WiFi.setSleep(false);
  esp_wifi_set_ps(WIFI_PS_NONE);
}, ARDUINO_EVENT_WIFI_STA_GOT_IP);
```

**Event enum naming:** Use `ARDUINO_EVENT_WIFI_STA_DISCONNECTED` (modern arduino-esp32 2.0.x). The older `SYSTEM_EVENT_STA_DISCONNECTED` name is deprecated but still defined as an alias — prefer the ARDUINO_ prefix.

### Anti-Patterns to Avoid
- **Prefix routing with `OSCBundle::route()` for colliding addresses:** `/ptz/pan` and `/ptz/pan/stop` share a prefix; `route("/ptz/pan", cb)` matches both and hands you the address offset, which means your velocity handler gets called for stop messages. Use exact `dispatch()` or `fullMatch()` instead.
- **Single-packet-per-loop parse:** Burst packets bunch; older velocities override newer ones. Always drain in a while loop.
- **Heap allocation per packet:** Do NOT `new OSCMessage` inside the receive loop. Stack-allocate: `OSCMessage msg;` inside `update()`.
- **Blocking reconnect logic in main loop:** Breaks motor step timing (though FastAccelStepper's ISR insulates motor timing, any multi-ms loop stall also blocks OSC reception → heartbeat auto-stop fires → motion halts mid-movement).
- **Logging inside tight OSC receive path without rate limiter:** `Serial.print` at 115200 baud = ~87µs/char. A 50-char log line = 4.3ms. Use `logShouldEmit()` on every new log point.
- **Forgetting `applySpeedAcceleration()` after `setAcceleration()`:** In continuous-run mode (`runForward/runBackward`), new accel values do NOT take effect until `applySpeedAcceleration()` is called. Existing `PtzMotion::setAxisVelocity()` already calls it — preserve this pattern in `applySpeedPreset()`.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| OSC message parse/encode | Custom binary parser | CNMAT/OSC `OSCMessage::fill()` + `dispatch()` | OSC has type-tag strings, 4-byte alignment, pattern matching — all spec edge cases |
| UDP socket | Raw lwIP calls | `WiFiUDP` from arduino-esp32 | Already bundled, correct integration with WiFi stack |
| WiFi reconnect loop | Polling `WiFi.status()` in main loop | `WiFi.onEvent(ARDUINO_EVENT_WIFI_STA_DISCONNECTED)` + `WiFi.reconnect()` | Non-blocking, driver-native, less state to track |
| Stepper acceleration curves | Computing velocity ramp yourself | `FastAccelStepper::setAcceleration()` + `applySpeedAcceleration()` | Hardware-timed (RMT/MCPWM); already working in Phase 1 |
| Heartbeat timer | FreeRTOS timer / software interrupt | `millis()` comparison in main loop | Single-threaded, simplest, works because motor timing runs in ISR (not affected by loop jitter) |
| Rate-limited logging | Ad-hoc `static uint32_t last = 0` per call site | Existing `ptz::logShouldEmit(LogRateId, intervalMs)` | Already implemented, LogRateId enum centralizes IDs |

**Key insight:** Every component for this phase already exists. The ONLY new code is the `ptz_osc` module (roughly 150 lines), tiny extensions to `ptz_motion` (~40 lines) and `ptz_wifi` (~15 lines), and config additions. No custom protocols, no custom schedulers.

## Common Pitfalls

### Pitfall 1: WiFi Power Save Re-enables After Reconnect
**What goes wrong:** Even after `WiFi.setSleep(false)` is set at initial connect, some arduino-esp32 releases have been observed to restore power-save defaults after a disconnect → reconnect cycle. OSC latency drops back to 100-300ms silently.
**Why it happens:** The WiFi driver reinitializes some fields on reconnect; `setSleep(false)` state is not always preserved.
**How to avoid:** Re-apply `WiFi.setSleep(false)` + `esp_wifi_set_ps(WIFI_PS_NONE)` inside the `ARDUINO_EVENT_WIFI_STA_GOT_IP` event handler. Belt-and-suspenders.
**Warning signs:** OSC responsiveness feels fine at boot but becomes sluggish after any WiFi hiccup.

### Pitfall 2: CNMAT/OSC Callback Signature Breaks Captures
**What goes wrong:** Developer writes `msg.dispatch("/ptz/pan", [this](OSCMessage& m){ … })` and gets a cryptic template-substitution compile error.
**Why it happens:** CNMAT `dispatch()` expects `void (*)(OSCMessage&)` — a raw function pointer. Capturing lambdas are NOT convertible.
**How to avoid:** Use one of three patterns:
1. File-static `PtzOsc* s_this = nullptr;` set in `begin()`; non-capturing lambdas / free functions dereference `s_this`.
2. Inline matching: `if (msg.fullMatch("/ptz/pan")) { onPan(msg); }` — avoids `dispatch()` entirely, slightly more verbose but no pointer gymnastics.
3. A small trampoline table of free functions.
**Warning signs:** Template errors like "cannot convert lambda to void (*)(OSCMessage&)".

### Pitfall 3: `OSCMessage::dispatch()` Does NOT Do Prefix Matching
**What goes wrong:** `msg.dispatch("/ptz/pan", cb)` does NOT fire when a `/ptz/pan/stop` message arrives — which is the CORRECT behavior — but the reverse trap is that `msg.route("/ptz", cb)` (prefix routing) WILL fire for both `/ptz/pan` and `/ptz/pan/stop`, dispatching the wrong handler.
**Why it happens:** `dispatch()` is exact-match; `route()` is prefix-match with offset. The locked OSC schema has `/ptz/pan` (float arg) and `/ptz/pan/stop` (no args) which ARE prefix collisions.
**How to avoid:** Use `dispatch()` exclusively in this phase. Verify by sending `/ptz/pan/stop` and confirming ONLY the stop handler fires (float-arg handler does NOT try to extract a non-existent arg).
**Warning signs:** Stop commands cause motor to start moving, or crashes on arg extraction from stop messages.

### Pitfall 4: CNMAT/OSC Heap Allocation Over Long Sessions
**What goes wrong:** CNMAT/OSC internally allocates `OSCData` objects on the heap during parsing. At 10-20Hz over a multi-hour session, fragmentation accumulates and eventually triggers allocation failures or watchdog resets.
**Why it happens:** Library is designed for desktop-class OSC; embedded heap constraints were not a primary design goal.
**How to avoid:** (1) Stack-allocate `OSCMessage msg;` per packet (do NOT `new`). (2) Log `ESP.getFreeHeap()` every 60s via a rate-limited log. (3) If heap trends DOWN over 30+ minutes, escalate to MicroOsc (research SUMMARY.md documents this fallback). Phase 2 acceptance criterion: heap stable within ±2KB over a 30-minute continuous-send test.
**Warning signs:** Uptime crashes after 12-48h; gradual decline in `ESP.getFreeHeap()` log output.

### Pitfall 5: Heartbeat Watchdog Fires Mid-Movement Due to OSC Rate Too Low
**What goes wrong:** Companion sends velocity at 5Hz while button held; 500ms heartbeat fires between packets; motor auto-stops → restarts → auto-stops, producing stutter.
**Why it happens:** 500ms timeout requires ≥2Hz send rate minimum, but some jitter demands ~10Hz for safety margin.
**How to avoid:** Document in CONTEXT.md (already done): "Companion must send at ≥10Hz while button held". Verify Companion generic-osc button-hold action is configured for continuous repeat at 100ms interval. Add a PTZ_LOGW on the first heartbeat fire per movement cycle to flag misconfiguration.
**Warning signs:** Stuttering motion on sustained button hold; heartbeat log fires repeatedly during button press.

### Pitfall 6: `setAcceleration()` Without `applySpeedAcceleration()` on Running Stepper
**What goes wrong:** Developer changes speed preset, calls `setAcceleration()` on each stepper, expects immediate change. Instead, the stepper keeps the old accel profile until the next `move()`/`moveTo()`/`applySpeedAcceleration()` call.
**Why it happens:** Documented FastAccelStepper behavior: "speed/acceleration can be varied while stepper is running (call to functions move or moveTo is needed in order to apply the new values)" — OR `applySpeedAcceleration()` for continuous-run mode.
**How to avoid:** After writing new `setSpeedInHz()` + `setAcceleration()` values in `applySpeedPreset()`, call `applySpeedAcceleration()` on each stepper IF it is currently running. If stopped, values take effect on next `runForward/runBackward`. Existing `setAxisVelocity()` already follows this pattern — copy it.
**Warning signs:** Preset switch "takes" only after operator releases and re-presses the button.

### Pitfall 7: `stopMove()` In Progress + New Velocity Command = Ignored Command
**What goes wrong:** User releases button → `stopMove()` starts decel → user presses again 100ms later → new velocity command arrives → stepper ignores it until decel completes.
**Why it happens:** FastAccelStepper docs: "no update on stopMove()" — parameters set during an active decel ramp don't apply until the ramp finishes.
**How to avoid:** On any incoming non-zero velocity for an axis that is currently decelerating, accept that there will be a small lag (~accel-time) before the new velocity takes effect. For Phase 2, document this and leave it — it's a physical constraint, not a bug. If it becomes a UX problem, switch to `forceStop()` + immediate `runForward()` sequence, but that produces a mechanical jolt.
**Warning signs:** Rapid direction-reversal feels "mushy" or lagged.

### Pitfall 8: UDP Receive Buffer Silently Drops Burst Packets
**What goes wrong:** Multiple simultaneous button holds in Companion produce bursts of OSC messages; ESP32's default UDP receive buffer (~4 packets) overflows; older packets get dropped silently without log.
**Why it happens:** arduino-esp32 WiFiUDP has a small default receive buffer; the WiFi driver can "bunch" packets when delivering.
**How to avoid:** Drain ALL packets per loop iteration (Pattern 2 above). Process only the MOST RECENT velocity per axis if multiple arrive in one drain (latest velocity supersedes). For Phase 2, simple processing (apply every command in order) is acceptable because handler work is trivial (~µs).
**Warning signs:** Motor response feels "chunky" or lags several hundred ms behind rapid button activity.

### Pitfall 9: Serial Logging of Every OSC Packet
**What goes wrong:** Debug logging like `PTZ_LOGI("OSC", "rx pan %.3f", val)` on every incoming packet. At 10Hz send rate × ~30 chars/line × 87µs/char @ 115200 baud = ~26ms/s spent blocking in Serial writes.
**Why it happens:** Developer adds logging for initial bring-up, forgets to rate-limit.
**How to avoid:** Use `logShouldEmit(kLogRateOscRxVelocity, 500)` — max once per 500ms. Error/warning logs (parse errors, heartbeat fires) can fire at 1-2s rate. Add new LogRateIds: `kLogRateOscRxVelocity`, `kLogRateOscUnknownAddr`, `kLogRateHeartbeatFired`, `kLogRateOscParseError`, `kLogRateWifiReconnect`, `kLogRateHeapFree`.
**Warning signs:** Motor response degrades or becomes jittery when debug logs enabled.

## Code Examples

Verified patterns from CNMAT/OSC documentation and existing Phase 1 code.

### OSC UDP Receive Loop with Dispatch
```cpp
// Source: CNMAT/OSC UDPReceive.ino example (adapted from ESP32 equivalent)
#include <WiFiUdp.h>
#include <OSCMessage.h>

WiFiUDP Udp;
void setup() { Udp.begin(8000); }

void loop() {
  OSCMessage msg;
  int size = Udp.parsePacket();
  if (size > 0) {
    while (size--) msg.fill(Udp.read());
    if (!msg.hasError()) {
      msg.dispatch("/ptz/pan", onPan);
      msg.dispatch("/ptz/stop", onStopAll);
    }
  }
}

void onPan(OSCMessage& m) {
  if (m.isFloat(0)) {
    float v = m.getFloat(0);
    // clamp & forward to motion
  }
}
```

### FastAccelStepper Runtime Speed Update (already in codebase)
```cpp
// Source: src/ptz_motion.cpp (Phase 1) — canonical pattern for applying new speed mid-run
void PtzMotion::setAxisVelocity(FastAccelStepper* stepper, float norm, float maxSps) {
  if (!stepper) return;
  if (fabsf(norm) < 0.001f) { stepper->stopMove(); return; }
  uint32_t speedHz = static_cast<uint32_t>(fabsf(norm) * maxSps);
  if (speedHz < 1) speedHz = 1;
  stepper->setSpeedInHz(speedHz);
  stepper->applySpeedAcceleration();   // critical for continuous-run
  if (norm > 0.0f) stepper->runForward();
  else stepper->runBackward();
}
```

### WiFi Event-Driven Reconnect
```cpp
// Source: arduino-esp32 docs + Phase 2 CONTEXT.md decisions
WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
  WiFi.reconnect();
}, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
```

### Power Save Disable (Phase 1 currently WRONG — replace)
```cpp
// CURRENT (Phase 1) — must be REMOVED:
//   WiFi.setSleep(true);
//   esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
// REPLACE WITH:
WiFi.setSleep(false);
esp_wifi_set_ps(WIFI_PS_NONE);
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `SYSTEM_EVENT_STA_DISCONNECTED` event name | `ARDUINO_EVENT_WIFI_STA_DISCONNECTED` | arduino-esp32 2.0.0 (2021) | Old name still aliased; prefer new name |
| AccelStepper polling in loop() | FastAccelStepper ISR-driven | Phase 1 (2026-04-05) | Motor timing decoupled from OSC/WiFi jitter — already done |
| `WiFi.setAutoReconnect(true)` | Event-handler + `WiFi.reconnect()` | arduino-esp32 >= 1.0.6 | Auto-reconnect unreliable per issue #653; event pattern is the accepted workaround |
| WebSocket + JSON control | OSC over UDP | Phase 1 (2026-04-05) | Already migrated — Phase 2 just adds the OSC transport |

**Deprecated/outdated:**
- `SYSTEM_EVENT_*` enum names: use `ARDUINO_EVENT_*` equivalents
- `WiFi.setAutoReconnect()`: known-unreliable; superseded by event handler pattern

## Open Questions

1. **Does CNMAT/OSC's `OSCMessage` destructor cleanly free all internal allocations per packet?**
   - What we know: Library uses internal `OSCData` heap objects; destructor is called when stack-allocated `OSCMessage msg;` goes out of scope.
   - What's unclear: Whether heap fragmentation accumulates despite proper cleanup, or if it's truly flat over long sessions.
   - Recommendation: Add `ESP.getFreeHeap()` monitoring (rate-limited log every 60s) as a Phase 2 implementation task. Phase 2 acceptance: heap delta within ±2KB over 30-minute continuous-send test. If not, escalate to MicroOsc in Phase 3.

2. **Optimal static UDP receive buffer size**
   - What we know: OSC messages for this phase are 16-32 bytes (e.g., `/ptz/pan` + `,f` + 4-byte float + padding = 24 bytes).
   - What's unclear: Whether to size the `uint8_t rxBuffer_[N]` at 128, 256, or rely purely on CNMAT's internal buffering via `msg.fill(byte)`.
   - Recommendation: Don't allocate a separate rxBuffer_ at all — feed `udp_.read()` bytes directly into `msg.fill()` as shown in pattern. Saves memory, matches CNMAT examples.

3. **Should serial PAN/TILT/ZOOM commands remain during Phase 2?**
   - What we know: They exist in Phase 1 `main.cpp` as a bring-up debug tool; useful for testing motion when WiFi unavailable.
   - Recommendation (Claude's discretion per CONTEXT): KEEP them for Phase 2. They're ~30 lines of code, zero runtime cost when idle, and provide a WiFi-independent debug path for hardware validation in Phase 4. Retire in Phase 3 or later if desired.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | PlatformIO `pio test` (ESP32 Unity) — NOT YET CONFIGURED in this repo |
| Config file | none — see Wave 0 |
| Quick run command | `pio run` (compile-only smoke; no on-target unit tests in Phase 2) |
| Full suite command | `pio run` + manual OSC send via `oscsend` / Protokol to hardware |

**Note:** This is embedded firmware with no host-side unit test infrastructure. "Validation" in Phase 2 is a combination of (a) compilation success, (b) scripted OSC-send tests against running hardware, (c) Phase 4 consolidates all hardware verification. The test map below uses **compilation + hardware smoke** as the automated layer.

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| NET-01 | WiFi connects via WiFiManager | smoke | `pio run` compile check; manual boot+serial log grep for "Connected" | Y (Phase 1) |
| NET-02 | Power save disabled | unit (code-review) | grep `"WIFI_PS_NONE"` in src/ptz_wifi.cpp | N — manual grep |
| NET-04 | UDP listener on port 8000 | smoke | compile + hardware: `oscsend osc.udp://ptzhead:8000 /ptz/pan f 0.5` observes motor move | N |
| NET-05 | Auto-reconnect on disconnect | manual-only | Unplug AP for 30s, observe reconnect via serial log after AP returns | manual — Phase 4 |
| MOT-01..03 | Per-axis velocity | hardware smoke | `oscsend … /ptz/{pan,tilt,zoom} f 0.5` → motor moves | N |
| MOT-04 | Per-axis stop | hardware smoke | send velocity then `/ptz/<axis>/stop` → that axis stops, others continue | N |
| MOT-05 | All-stop | hardware smoke | send all velocities then `/ptz/stop` → all stop smoothly | N |
| MOT-06 | Smooth accel/decel | manual (visual) | observe motor ramp; oscilloscope step pin at decel — Phase 4 | manual |
| MOT-07 | Heartbeat auto-stop | hardware smoke | send velocity, stop sending, verify motor decels to stop within ~500ms | N |
| SPD-01 | ≥3 presets switchable | hardware smoke | `oscsend … /ptz/speed/preset i 0/1/2`, send velocity, observe 3 distinct speeds | N |
| SPD-02 | Presets affect vel AND accel | manual (visual/timing) | observe ramp time differs between slow/fast presets — Phase 4 | manual |
| SPD-03 | Preset persists | hardware smoke | set preset 0, send multiple velocities, verify all use preset 0 | N |

### Sampling Rate
- **Per task commit:** `pio run` (compile check, <30s)
- **Per wave merge:** `pio run` + bench smoke via `oscsend` scripts (manual, <5 min)
- **Phase gate:** Full hardware smoke suite green before `/gsd:verify-work`; MOT-06 / SPD-02 / NET-05 manual checks DEFERRED to Phase 4 per ROADMAP.md

### Wave 0 Gaps
- [ ] `scripts/osc_smoke.sh` — wraps `oscsend` for repeatable per-req hardware smoke tests (NET-04, MOT-01..05, MOT-07, SPD-01, SPD-03)
- [ ] `scripts/osc_heap_soak.sh` — sends continuous velocity at 10Hz for 30 minutes, captures serial `ESP.getFreeHeap()` log for CNMAT heap-stability acceptance (Open Question 1)
- [ ] No test framework install required — `pio run` exists; OSC tooling (`oscsend` from liblo, or Protokol GUI) is the operator's workstation dependency, not the firmware's

## Sources

### Primary (HIGH confidence)
- Context7 / official docs: Not queried (Context7 unavailable in this environment) — relied on fetched GitHub sources
- [CNMAT/OSC library.json master](https://github.com/CNMAT/OSC/blob/master/library.json) — version 3.5.8 verified 2026-04-05
- [CNMAT/OSC UDPReceive example](https://github.com/CNMAT/OSC/blob/master/examples/UDPReceive/UDPReceive.ino) — canonical receive + dispatch/route pattern
- [FastAccelStepper README](https://github.com/gin66/FastAccelStepper/blob/master/README.md) — stopMove/forceStop/runtime speed change semantics
- [FastAccelStepper API docs](https://github.com/gin66/FastAccelStepper/blob/master/extras/doc/FastAccelStepper_API.md) — `applySpeedAcceleration()` semantics, "no update on stopMove()" constraint
- Existing codebase (Phase 1): `src/main.cpp`, `src/ptz_motion.cpp`, `src/ptz_wifi.cpp`, `src/ptz_config.h`, `platformio.ini`
- Project research artifacts: `.planning/research/STACK.md`, `.planning/research/PITFALLS.md`, `.planning/research/ARCHITECTURE.md`, `.planning/research/SUMMARY.md` (all researched 2026-04-03, HIGH confidence)

### Secondary (MEDIUM confidence)
- [arduino-esp32 issue #653 — WiFi auto-reconnect broken](https://github.com/espressif/arduino-esp32/issues/653) — cited in PITFALLS.md
- [arduino-esp32 issue #4406 — mDNS 2-min expiry](https://github.com/espressif/arduino-esp32/issues/4406) — Phase 3 concern
- [ESP32 WiFi power save docs](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/wifi-driver/wifi-performance-and-power-save.html) — WIFI_PS_NONE behavior
- [Companion generic-osc module](https://github.com/bitfocus/companion-module-generic-osc) — UDP port 8000 default confirmed

### Tertiary (LOW confidence)
- CNMAT/OSC heap behavior over multi-hour sessions: not directly measured in this specific hardware config — flagged as Open Question 1, requires runtime monitoring per Pitfall 4

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — CNMAT/OSC version verified live against GitHub; FastAccelStepper + WiFiManager already in use and working in Phase 1
- Architecture: HIGH — patterns come from existing codebase conventions plus canonical CNMAT examples; `PtzOsc` module sketch in ARCHITECTURE.md maps cleanly to locked decisions
- Pitfalls: HIGH — nine concrete pitfalls backed by GitHub issues, library docs, or direct existing-code patterns; no theoretical risks
- CNMAT dispatch vs route semantics: HIGH — verified from CNMAT UDPReceive.ino + address-collision analysis specific to locked `/ptz/pan` + `/ptz/pan/stop` schema

**Research date:** 2026-04-05
**Valid until:** 2026-05-05 (30 days — stable embedded stack, no fast-moving dependencies)
