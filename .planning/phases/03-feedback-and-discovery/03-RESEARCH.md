# Phase 3: Feedback and Discovery - Research

**Researched:** 2026-04-05
**Domain:** ESP32 OSC TX feedback + mDNS service advertisement (Arduino-ESP32, CNMAT/OSC, ESPmDNS)
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

**Feedback address schema (FB-01, FB-02, FB-03, FB-04)**
- Separate scalar addresses under `/ptz/status/...`:
  - `/ptz/status/pan/moving i` — 0 or 1
  - `/ptz/status/tilt/moving i` — 0 or 1
  - `/ptz/status/zoom/moving i` — 0 or 1
  - `/ptz/status/preset i` — active preset index (0..kPresetCount-1)
  - `/ptz/status/rssi i` — WiFi RSSI in dBm (negative int, e.g. -62)
- One scalar per address to match Companion generic-osc's one-variable-per-address model.
- All values are integers on the wire (FB-04). No floats, no bundles.

**Send cadence**
- Mixed: state-on-change + metrics-periodic.
- **On-change** (emit immediately when value flips):
  - per-axis `moving` flags — derived from `PtzMotion::isMoving()` split per axis
  - `preset` — emit from inside `applySpeedPreset()` ack path or detected via change in main loop
- **Periodic every 1000ms**:
  - `rssi` — read `WiFi.RSSI()` and send
  - Also re-emit full state snapshot every 1s as self-heal against packet loss (cheap, 5 ints).
- Feedback emission happens from the main loop (not ISR); PtzOsc owns a small timer.

**Reply target**
- **Reply-to-sender**: `PtzOsc::update()` caches `s_udp.remoteIP()` and `s_udp.remotePort()` from last successfully parsed packet into `lastSender_` (IPAddress + uint16_t port).
- Feedback sends go to `lastSender_`.
- If no packet has been received yet, feedback is silently suppressed (no broadcast fallback in v1).
- Single Companion sender assumption holds from Phase 2.

**mDNS advertisement (NET-03)**
- Hostname: `ptzhead.local` (locked by requirement).
- Service: `_osc._udp` on port `kOscPort` (8000).
- Service instance name: `PTZHead`.
- **No TXT records** in v1.
- Lifecycle: start `MDNS.begin("ptzhead")` + `MDNS.addService(...)` on `ARDUINO_EVENT_WIFI_STA_GOT_IP`. On reconnect, `MDNS.end()` then begin again. Piggyback on existing event-driven WiFi handler from Phase 2.

**Heartbeat safety invariant**
- `PtzOsc::lastRxMs_` stays RX-only. The send path must NOT touch it.
- 500ms heartbeat watchdog unchanged.

### Claude's Discretion

- Per-axis `isMoving()` split: add `isPanMoving()`, `isTiltMoving()`, `isZoomMoving()` (or `axisMoving(AxisId)`) to `PtzMotion`.
- Where feedback lives: extend `PtzOsc` with `sendStatus()` methods vs. new `PtzFeedback` class. Leaning: extend `PtzOsc`.
- Change detection bookkeeping: cache last-sent values in PtzOsc and diff each tick vs. push notifications. Leaning: cache+diff in PtzOsc.
- Whether to coalesce 1s periodic snapshot with on-change into single code path.
- New LogRateIds for feedback TX events.
- Whether mDNS logic warrants own module (`ptz_mdns`) or stays inside `ptz_wifi`.

### Deferred Ideas (OUT OF SCOPE)

- TXT records (fw version, axes, active preset)
- Fixed-IP feedback fallback for pre-RX state
- Broadcast feedback for multi-listener scenarios
- Separate outbound port
- Floats on the wire (explicitly rejected by FB-04)
- Multi-target fanout (several Companions)
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| NET-03 | Firmware advertises via mDNS as `ptzhead.local` with `_osc._udp` service | ESPmDNS API section, mDNS lifecycle pattern, Pitfall 4 mitigation (WIFI_PS_NONE already set in Phase 2, restart on GOT_IP) |
| FB-01 | Per-axis moving state back via OSC (integer 0/1) | CNMAT OSC TX pattern, per-axis `isMoving()` split, change-detection diff/cache pattern |
| FB-02 | Active speed preset ID back via OSC (integer) | On-change emission from `applySpeedPreset()` + 1s snapshot re-emit |
| FB-03 | WiFi RSSI back via OSC (integer) | `WiFi.RSSI()` returns `int8_t`, sent as OSC `i` (int32) every 1000ms |
| FB-04 | All OSC feedback uses integer values | `OSCMessage::add(int32_t)` type tag `i`, verified CNMAT API |

</phase_requirements>

## Summary

Phase 3 adds outbound OSC feedback and mDNS discovery on top of the Phase 2 RX-only OSC listener. All five feedback values are scalar integers on distinct `/ptz/status/...` addresses so Companion generic-osc can bind one variable per address. Two mechanisms co-exist: on-change emission (moving flags and preset flip immediately) and a 1-second periodic snapshot that re-emits every value (self-heal against UDP loss and Companion restart). Outbound packets target the cached IP/port from the last successfully parsed OSC packet — no broadcast, no fixed IP.

mDNS uses the built-in `ESPmDNS` library (part of arduino-esp32 core, no `lib_deps` change). It advertises `ptzhead.local` with service type `_osc._udp` on port 8000. The known ESP32 mDNS "stops after 2 minutes" bug is already mitigated by Phase 2's `WIFI_PS_NONE` setting; additionally the decision to call `MDNS.end()` + `MDNS.begin()` on every GOT_IP event keeps the service fresh across reconnects. CNMAT/OSC's TX path uses the same `OSCMessage` type as RX — `msg.add(int32_t).send(udp)` wrapped in `beginPacket/endPacket` — so zero new dependencies.

**Primary recommendation:** Extend `PtzOsc` with a `updateFeedback()` method called from the main loop after `osc.update()`. Cache last-sent values in a `lastSent_` struct, diff on every tick against a fresh snapshot pulled from `PtzMotion` + `WiFi.RSSI()`, emit changed values immediately and re-emit all five values every 1000ms. Add `MDNS.begin("ptzhead")` + `MDNS.addService("_osc", "_udp", 8000)` + `MDNS.setInstanceName("PTZHead")` inside the existing GOT_IP event handler in `ptz_wifi.cpp`, calling `MDNS.end()` first to handle reconnect cleanly.

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| CNMAT/OSC | 3.5.8 (already in lib_deps) | OSC message parse + build + TX | Phase 2 locked this choice; reuses existing `OSCMessage` type for send path |
| ESPmDNS | bundled with arduino-esp32 core (espressif32 @ 6.10.0) | mDNS hostname + service advertisement | Official Espressif/Arduino library, included with framework, no `lib_deps` entry |
| WiFi (arduino-esp32) | bundled | `WiFi.RSSI()` for signal strength, event handlers | Core library, already in use |
| WiFiUdp | bundled | UDP send via `beginPacket`/`endPacket` | Already used for RX; extend for TX |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| (none) | — | — | No new libraries required |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| CNMAT OSCMessage TX | hand-rolled raw OSC byte builder | Saves ~1KB heap but duplicates CNMAT's type-tag logic; rejected — reuse existing dep |
| ESPmDNS | `<mdns.h>` ESP-IDF raw API | More control but bypasses Arduino wrapper Phase 2 already targets; rejected |
| Single combined bundle `/ptz/status` | separate scalar addresses | Companion generic-osc binds one variable per address — scalars match the model; locked by user |

**Installation:**

No package installation needed. `ESPmDNS` is part of the arduino-esp32 core (already pulled in by `framework = arduino` + `platform = espressif32@6.10.0` from Phase 1). Include with:

```cpp
#include <ESPmDNS.h>
```

**Version verification:**
- CNMAT/OSC 3.5.8 — locked from Phase 2, `lib_deps` already references GitHub tag.
- ESPmDNS — follows arduino-esp32 core version. As of arduino-esp32 3.x (April 2026), the API signatures used here (`begin`, `end`, `addService`, `setInstanceName`) are stable. Verified against upstream `libraries/ESPmDNS/src/ESPmDNS.h` on master branch.

## Architecture Patterns

### Recommended Project Structure
```
src/
├── ptz_osc.{h,cpp}       # EXTEND: add TX path, sender cache, periodic tick, status cache+diff
├── ptz_motion.{h,cpp}    # EXTEND: add per-axis isPanMoving/isTiltMoving/isZoomMoving accessors
├── ptz_wifi.{h,cpp}      # EXTEND: hook MDNS.begin into GOT_IP event handler
├── ptz_config.h          # EXTEND: kFeedbackPeriodMs, mDNS constants, new LogRateIds
├── ptz_mdns.{h,cpp}      # OPTIONAL: new module if mDNS logic grows beyond 10 lines
└── main.cpp              # EXTEND: call g_osc.updateFeedback() in loop() after g_osc.update()
```

### Pattern 1: Reply-to-Sender Cache in RX Path
**What:** Stash `remoteIP()` / `remotePort()` on every successfully parsed packet; use that tuple as the TX destination.
**When to use:** Single-client request/response over UDP where client's source port is ephemeral (Companion generic-osc does not bind a fixed source port).
**Example:**
```cpp
// src/ptz_osc.h — additions
class PtzOsc {
 public:
  void updateFeedback();
  bool hasSender() const { return lastSenderPort_ != 0; }
 private:
  IPAddress lastSenderIp_;
  uint16_t lastSenderPort_ = 0;
  // ... existing members
};

// src/ptz_osc.cpp — inside update() after successful parse
void PtzOsc::update() {
  int size;
  while ((size = s_udp.parsePacket()) > 0) {
    // Capture sender IMMEDIATELY — before reading, because parsePacket()
    // establishes the remote endpoint for the just-received datagram.
    IPAddress remoteIp = s_udp.remoteIP();
    uint16_t remotePort = s_udp.remotePort();

    OSCMessage msg;
    while (size--) msg.fill(s_udp.read());
    if (msg.hasError()) { /* existing */ continue; }

    lastRxMs_ = millis();         // RX-only, heartbeat-owned
    lastSenderIp_ = remoteIp;     // TX target for feedback
    lastSenderPort_ = remotePort;
    dispatchAll(msg);
  }
}
```

### Pattern 2: CNMAT OSCMessage TX via WiFiUDP
**What:** Build `OSCMessage` with address + int arg, wrap `msg.send(udp)` in `beginPacket`/`endPacket`.
**When to use:** Every outbound scalar-int OSC packet.
**Example:**
```cpp
// Source: https://github.com/CNMAT/OSC/blob/master/examples/UDPSendMessage/UDPSendMessage.ino
void PtzOsc::sendScalarInt(const char* addr, int32_t value) {
  if (!hasSender()) return;  // no RX yet, suppress
  OSCMessage msg(addr);
  msg.add(value);            // int32 → OSC 'i' type tag
  s_udp.beginPacket(lastSenderIp_, lastSenderPort_);
  msg.send(s_udp);
  s_udp.endPacket();
  msg.empty();               // free internal allocations (Pitfall 6 mitigation)
}
```

### Pattern 3: Cache + Diff for On-Change Emission
**What:** Keep a `lastSent_` struct; on each tick, build a fresh snapshot and compare field-by-field; emit only changed fields plus a full snapshot every N ms.
**When to use:** When downstream consumer (Companion) only needs to see transitions and periodic recovery, not the full rate of state churn.
**Example:**
```cpp
// src/ptz_osc.h
struct StatusSnapshot {
  int32_t panMoving  = 0;
  int32_t tiltMoving = 0;
  int32_t zoomMoving = 0;
  int32_t preset     = -1;  // sentinel: force first emit
  int32_t rssi       = 0;
};

// src/ptz_osc.cpp
void PtzOsc::updateFeedback() {
  if (!hasSender()) return;
  const uint32_t now = millis();
  const bool periodicTick = (now - lastFeedbackMs_) >= kFeedbackPeriodMs;

  StatusSnapshot cur;
  cur.panMoving  = motion_->isPanMoving()  ? 1 : 0;
  cur.tiltMoving = motion_->isTiltMoving() ? 1 : 0;
  cur.zoomMoving = motion_->isZoomMoving() ? 1 : 0;
  cur.preset     = motion_->activePreset();
  cur.rssi       = WiFi.RSSI();  // int8_t, promoted to int32

  // On-change: emit any field that flipped since last_.
  if (cur.panMoving  != last_.panMoving)  sendScalarInt("/ptz/status/pan/moving",  cur.panMoving);
  if (cur.tiltMoving != last_.tiltMoving) sendScalarInt("/ptz/status/tilt/moving", cur.tiltMoving);
  if (cur.zoomMoving != last_.zoomMoving) sendScalarInt("/ptz/status/zoom/moving", cur.zoomMoving);
  if (cur.preset     != last_.preset)     sendScalarInt("/ptz/status/preset",     cur.preset);

  // Periodic snapshot (self-heal): re-emit ALL five every 1000ms.
  if (periodicTick) {
    sendScalarInt("/ptz/status/pan/moving",  cur.panMoving);
    sendScalarInt("/ptz/status/tilt/moving", cur.tiltMoving);
    sendScalarInt("/ptz/status/zoom/moving", cur.zoomMoving);
    sendScalarInt("/ptz/status/preset",      cur.preset);
    sendScalarInt("/ptz/status/rssi",        cur.rssi);
    lastFeedbackMs_ = now;
  }

  last_ = cur;
}
```

### Pattern 4: mDNS Lifecycle on GOT_IP Event
**What:** Restart mDNS on every `ARDUINO_EVENT_WIFI_STA_GOT_IP` — fresh registration after every IP assignment.
**When to use:** When mDNS reliability matters and WiFi can reconnect; `end()` + `begin()` is cheap.
**Example:**
```cpp
// src/ptz_wifi.cpp — inside registerWifiEventHandlers(), GOT_IP branch
WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
  PTZ_LOGI("WIFI", "GOT_IP %s", WiFi.localIP().toString().c_str());
  WiFi.setSleep(false);
  esp_wifi_set_ps(WIFI_PS_NONE);

  // mDNS: tear down any prior instance, then restart fresh.
  MDNS.end();
  if (MDNS.begin(kMdnsHostname)) {             // "ptzhead" → ptzhead.local
    MDNS.setInstanceName(kMdnsInstanceName);   // "PTZHead"
    MDNS.addService("_osc", "_udp", kOscPort); // 8000
    PTZ_LOGI("MDNS", "advertising %s.local _osc._udp:%u", kMdnsHostname, kOscPort);
  } else {
    PTZ_LOGW("MDNS", "begin failed");
  }
}, ARDUINO_EVENT_WIFI_STA_GOT_IP);
```

### Anti-Patterns to Avoid

- **Sending floats for FB-04 values:** FB-04 explicitly mandates integers. Companion generic-osc tolerates floats but user has locked int-only.
- **Touching `lastRxMs_` from the send path:** The heartbeat is silence-of-RX, not silence-of-TX. Any write to `lastRxMs_` outside `PtzOsc::update()` parse-success path is a safety regression.
- **Bundling all five values into one OSC bundle:** Companion generic-osc binds per-address, doesn't unpack bundles cleanly. Use five separate messages.
- **Emitting feedback every loop iteration:** Loop runs at kHz rates; without rate limiting you'd flood the network and fragment heap. Use the 1s periodic + on-change model.
- **Broadcasting to 255.255.255.255:** User rejected broadcast fallback; silently suppress when `lastSenderPort_ == 0`.
- **Calling `MDNS.begin()` before WiFi has an IP:** mDNS requires a bound IP interface. Must be inside GOT_IP event, not in `setup()`.
- **Forgetting `msg.empty()` after send:** CNMAT `OSCMessage` holds heap-allocated OSCData children; leaking these accelerates Pitfall 6 (heap fragmentation).

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| OSC message serialization | custom byte-packer for addr + type tag + int32 | `OSCMessage(addr).add(int).send(udp)` | CNMAT already handles type tag padding, 4-byte alignment, big-endian int32. Rebuilding duplicates Phase 2 dep. |
| mDNS packet construction | raw UDP multicast on 224.0.0.251 | `ESPmDNS` library | mDNS spec (RFC 6762) has TTL, probing, conflict resolution rules — don't reimplement. |
| Change detection | per-field dirty flags set from PtzMotion push-notifications | single `lastSent_` struct diffed each tick | Fewer callpaths, one owner, trivially auditable; user explicitly leans this way. |
| Sender-IP lookup | ARP table scanning or config file | `s_udp.remoteIP()` / `remotePort()` after `parsePacket()` | WiFiUdp already exposes this; it's the canonical reply-to-sender pattern. |
| Periodic scheduler | FreeRTOS timer or ticker | `millis() - lastFeedbackMs_ >= kFeedbackPeriodMs` check in main loop | Phase 2 established non-blocking `millis()` polling as the project pattern; matches heartbeat style. |

**Key insight:** Every line of bespoke OSC/mDNS/timer code is another surface for Pitfalls 3/4/6/8. The standard libraries already make the right tradeoffs for this embedded context.

## Common Pitfalls

### Pitfall 1: ESP32 mDNS Stops Responding After ~2 Minutes
**What goes wrong:** After boot + successful `MDNS.begin()`, service goes silent within 2-5 minutes; `ping ptzhead.local` stops resolving.
**Why it happens:** Arduino-ESP32 mDNS bug (#4406, #7156) combined with WiFi power save missing incoming mDNS queries during sleep.
**How to avoid:**
- `WIFI_PS_NONE` already set in Phase 2 — do NOT re-enable power save.
- Call `MDNS.end()` + `MDNS.begin()` on every GOT_IP event (even on reconnects) — acts as a "kick" that restarts the responder fresh.
- Always call `MDNS.addService("_osc", "_udp", port)` — without a service, the responder can go dormant.
**Warning signs:** `ping ptzhead.local` works at boot but fails 3 minutes later from laptop on same LAN.

### Pitfall 2: CNMAT OSCMessage Heap Fragmentation on TX Path
**What goes wrong:** Creating + destroying `OSCMessage` objects every tick (up to 5/second steady-state, more during change bursts) fragments the ESP32 heap over hours.
**Why it happens:** `OSCMessage::add()` allocates OSCData on heap. `msg.empty()` returns chunks to the allocator, but fragmentation accumulates.
**How to avoid:**
- ALWAYS call `msg.empty()` immediately after `send()`.
- Monitor `ESP.getFreeHeap()` trend (already done via `kLogRateHeapFree` every 60s from Phase 2).
- Keep TX payload minimal: one int per message, no nested structures.
- If 24h uptime shows declining trend, consider reusing a single stack-allocated `OSCMessage` with `.empty()` + re-add pattern (future optimization).
**Warning signs:** `ESP.getFreeHeap()` trends downward ~500 bytes/hour; watchdog reset after 12-48h uptime.

### Pitfall 3: Stale Sender Cache After WiFi Reconnect
**What goes wrong:** After WiFi drops and reconnects with a new DHCP lease (or Companion restarts with new ephemeral port), `lastSenderIp_` / `lastSenderPort_` point at a stale endpoint. Packets go to void.
**Why it happens:** The cached tuple is last-seen, not validated-live. Nothing clears it on disconnect.
**How to avoid:**
- Natural self-healing: as soon as Companion sends its next OSC command, the cache refreshes with the new source tuple.
- Acceptable v1 behavior: feedback dark for up to ~2 seconds after reconnect, then recovers on next RX.
- Optional hardening: clear `lastSenderPort_ = 0` on `ARDUINO_EVENT_WIFI_STA_DISCONNECTED` to force "suppress until next RX" explicitly.
**Warning signs:** After WiFi hiccup, Companion shows stale variable values until operator presses a button.

### Pitfall 4: WiFi.RSSI() Returns 0 or 31 When Not Connected
**What goes wrong:** Emitting feedback while WiFi is disconnected returns garbage RSSI values; Companion displays them.
**Why it happens:** `WiFi.RSSI()` is only valid when `WiFi.status() == WL_CONNECTED`.
**How to avoid:**
- `updateFeedback()` gates on `hasSender()` which implies a recent RX, which implies WiFi is up. Natural gate.
- Optional: explicit `if (WiFi.status() != WL_CONNECTED) return;` at top of `updateFeedback()` for belt-and-suspenders.
**Warning signs:** Companion shows RSSI=0 or RSSI=31 during WiFi dropout.

### Pitfall 5: `remoteIP()` / `remotePort()` Called After Reading Packet Data
**What goes wrong:** Some UDP implementations only expose remote endpoint info during/after `parsePacket()` but before the internal buffer is drained. Order matters.
**Why it happens:** WiFiUdp's remote state is tied to the packet currently being read.
**How to avoid:** Capture `remoteIP()` and `remotePort()` IMMEDIATELY after `parsePacket()` returns > 0, BEFORE the `while (size--) msg.fill(s_udp.read())` drain loop. See Pattern 1.
**Warning signs:** Feedback targets 0.0.0.0 or port 0; `hasSender()` never returns true.

### Pitfall 6: Preset Change Emit Timing vs. Apply Timing
**What goes wrong:** Emitting `/ptz/status/preset` from inside `applySpeedPreset()` couples `PtzMotion` to `PtzOsc`, creating a reverse dependency.
**Why it happens:** Temptation to emit at the point of change for lowest latency.
**How to avoid:** Keep emission in `PtzOsc::updateFeedback()` where it diffs `activePreset()` against `last_.preset`. Added latency is one loop iteration (~<1ms), indistinguishable from ideal.
**Warning signs:** `#include "ptz_osc.h"` appears in `ptz_motion.cpp` — architectural smell.

### Pitfall 7: Log Spam From Feedback TX
**What goes wrong:** Logging every outbound packet at 5/sec + on-change bursts floods Serial, introduces jitter (Pitfall 10 from Phase 2 PITFALLS.md).
**How to avoid:**
- Add new `kLogRateFeedbackTx` LogRateId, rate-limited to once per 2000ms minimum.
- Log only on first successful sender-cache populate, and on mDNS start — not per-packet.
**Warning signs:** Serial console scrolls continuously during idle operation.

## Code Examples

Verified patterns from official sources:

### CNMAT OSCMessage UDP Send
```cpp
// Source: https://github.com/CNMAT/OSC/blob/master/examples/UDPSendMessage/UDPSendMessage.ino
OSCMessage msg("/ptz/status/rssi");
msg.add((int32_t)WiFi.RSSI());
Udp.beginPacket(outIp, outPort);
msg.send(Udp);      // OSCMessage::send(Print&) — WiFiUDP inherits Print via Stream
Udp.endPacket();
msg.empty();        // CRITICAL: free internal OSCData heap
```

### ESPmDNS begin + service advertisement
```cpp
// Source: https://github.com/espressif/arduino-esp32/blob/master/libraries/ESPmDNS/src/ESPmDNS.h
#include <ESPmDNS.h>

if (!MDNS.begin("ptzhead")) {
  // failed — WiFi may not be up yet
  return;
}
MDNS.setInstanceName("PTZHead");
MDNS.addService("_osc", "_udp", 8000);
// Device is now discoverable as ptzhead.local advertising _osc._udp:8000
```

### WiFiUDP remote endpoint capture
```cpp
// Source: Arduino WiFiUdp API (arduino-esp32 core)
int size = udp.parsePacket();
if (size > 0) {
  IPAddress sender = udp.remoteIP();    // must be called after parsePacket() > 0
  uint16_t port = udp.remotePort();     //   and before draining the buffer
  // ... read packet data ...
}
```

### Per-axis isMoving() using FastAccelStepper
```cpp
// Source: FastAccelStepper README — isRunning() is the canonical "is this axis moving?" query
bool PtzMotion::isPanMoving()  const { return pan_  && pan_->isRunning(); }
bool PtzMotion::isTiltMoving() const { return tilt_ && tilt_->isRunning(); }
bool PtzMotion::isZoomMoving() const { return zoom_ && zoom_->isRunning(); }
// (Note: existing isMoving() becomes logical OR of these three.)
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `MDNS.setInstanceName` optional | Always set for readable instance in Bonjour browser | arduino-esp32 2.x+ | "PTZHead" appears in Bonjour Browser / Discovery, not just "ptzhead" |
| Manual mDNS re-announce loop | Rely on `MDNS.addService()` + restart on reconnect | arduino-esp32 2.x stable | Fewer moving parts; restart-on-GOT_IP pattern is the community consensus |
| OSC floats for everything | Integer-only for Companion generic-osc compatibility | Companion 3.x generic-osc behavior | Locked by FB-04; all Phase 3 TX is `i` type tag |

**Deprecated/outdated:**
- `MDNS.announce()` — not exposed in current arduino-esp32 ESPmDNS API (was in some older forks). Don't depend on it.
- Bundle-based OSC status messages — Companion generic-osc unpacks per-address, bundles add complexity without benefit.

## Open Questions

1. **Should `lastSenderPort_ = 0` be cleared on disconnect event?**
   - What we know: Natural self-healing on next RX is acceptable (up to 2s dark period).
   - What's unclear: Whether operators find a 2s post-reconnect feedback gap annoying enough to warrant explicit clearing.
   - Recommendation: Ship without explicit clear (simpler); revisit in Phase 4 hardware validation if user complains.

2. **Is a separate `ptz_mdns.{h,cpp}` module warranted?**
   - What we know: mDNS setup is ~6 lines of code inside the GOT_IP event.
   - What's unclear: Whether it will grow with TXT records or multi-service in v2.
   - Recommendation: Inline in `ptz_wifi.cpp` for v1 (user's discretion area); promote to own module only if it exceeds ~15 lines.

3. **Should `updateFeedback()` early-exit on `WiFi.status() != WL_CONNECTED`?**
   - What we know: `hasSender()` gate implicitly covers this.
   - What's unclear: Edge case where sender cache is populated but WiFi just dropped mid-loop.
   - Recommendation: Add explicit `WiFi.status() == WL_CONNECTED` check for defense-in-depth; cheap.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | None configured yet (Arduino/PlatformIO project) — behavioral validation via host loopback script or Phase 4 hardware |
| Config file | none — see Wave 0 |
| Quick run command | `pio run` (compile-only validation) |
| Full suite command | `pio run && pio run -t checkprogsize` + manual loopback (see Wave 0) |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| NET-03 | `ptzhead.local` resolves + advertises `_osc._udp:8000` | integration (host) | `dns-sd -B _osc._udp local.` (macOS) or `avahi-browse -r _osc._udp` (Linux) after flash | manual-only |
| FB-01 | `/ptz/status/pan/moving i {0,1}` flips on axis motion | integration (loopback) | Python loopback script: send `/ptz/pan 0.5`, listen on ephemeral port, expect `/ptz/status/pan/moving 1` within 100ms | ❌ Wave 0 |
| FB-01 | `/ptz/status/tilt/moving` + `/ptz/status/zoom/moving` same contract | integration (loopback) | same loopback script, extend for tilt + zoom | ❌ Wave 0 |
| FB-02 | `/ptz/status/preset i N` emitted on preset change | integration (loopback) | loopback: send `/ptz/speed/preset 2`, expect `/ptz/status/preset 2` | ❌ Wave 0 |
| FB-03 | `/ptz/status/rssi i` emitted every 1000ms | integration (loopback) | loopback: listen 3s, expect ≥2 RSSI messages with negative int value | ❌ Wave 0 |
| FB-04 | All feedback values are OSC int32 type tag `i` | unit (message inspection) | loopback script asserts type tag byte after address+padding | ❌ Wave 0 |
| NET-03 | mDNS survives >5 minutes | integration (host, long-running) | `while true; do dig @224.0.0.251 -p 5353 ptzhead.local; sleep 30; done` over 10min | manual-only |

**Note on manual-only:** NET-03 requires an IP-stack interaction that can't run on CI without a live ESP32 on the same L2. Phase 4 (hardware validation) will cover this on real hardware. Loopback tests for FB-01..FB-04 CAN run on CI against the firmware flashed to a dev board via a small Python OSC script, but since this is an Arduino/PlatformIO project with no existing test harness, they'll run as manual steps during task verification.

### Sampling Rate
- **Per task commit:** `pio run` (compile + link clean)
- **Per wave merge:** `pio run` + visual inspection of `ptz_osc.cpp` diff for `lastRxMs_` touches (heartbeat invariant guard)
- **Phase gate:** Manual loopback test with Python OSC client (see Wave 0 script) + `dns-sd -B _osc._udp local.` on macOS host before `/gsd:verify-work`

### Wave 0 Gaps
- [ ] `scripts/feedback_loopback_test.py` — Python OSC client that connects to `ptzhead.local:8000`, sends motion commands, listens on ephemeral port, asserts feedback addresses + int type tags + values. Uses `python-osc` package.
- [ ] `scripts/README.md` — document how to run the loopback test against a live ESP32
- [ ] (Optional) Framework install: none — project intentionally has no unit test framework; Arduino/PlatformIO + hardware loopback is the validation model.

## Sources

### Primary (HIGH confidence)
- **CNMAT/OSC repository** — https://github.com/CNMAT/OSC — `OSCMessage.h` API (`add<T>`, `send(Print&)`, `empty()`), UDP send example
- **CNMAT OSC UDPSendMessage example** — https://github.com/CNMAT/OSC/blob/master/examples/UDPSendMessage/UDPSendMessage.ino — canonical beginPacket/send/endPacket/empty pattern
- **arduino-esp32 ESPmDNS header** — https://github.com/espressif/arduino-esp32/blob/master/libraries/ESPmDNS/src/ESPmDNS.h — `begin`, `end`, `addService`, `setInstanceName` signatures
- **Phase 2 RESEARCH + PITFALLS** — `.planning/research/PITFALLS.md` pitfalls 1, 3, 4, 6, 8, 10 all apply directly
- **Phase 2 CONTEXT** — `.planning/phases/02-network-and-core-osc-control/02-CONTEXT.md` locked CNMAT 3.5.8, WIFI_PS_NONE, single-sender assumption
- **Existing codebase** — `src/ptz_osc.{h,cpp}`, `src/ptz_motion.{h,cpp}`, `src/ptz_wifi.cpp`, `src/ptz_config.h` — direct inspection

### Secondary (MEDIUM confidence)
- **arduino-esp32 issue #4406** — mDNS stops after 2 minutes (noted in Phase 2 PITFALLS)
- **arduino-esp32 issue #7156** — mDNS unreliable with WiFi sleep (mitigation already in place)
- **Bitfocus Companion generic-osc behavior** — one-variable-per-address binding model (Phase 2 research)

### Tertiary (LOW confidence)
- None — all critical claims verified against official sources or existing code.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — all libraries already in build, APIs verified against upstream headers
- Architecture: HIGH — patterns derived from Phase 2 code + official CNMAT/ESPmDNS examples
- Pitfalls: HIGH — 4 of 7 pitfalls carry over from Phase 2 PITFALLS.md (already validated in-domain), 3 are direct consequences of the new TX path

**Research date:** 2026-04-05
**Valid until:** 2026-05-05 (30 days; stable libraries, well-understood ESP32 mDNS quirks)
