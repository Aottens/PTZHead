# Phase 3: Feedback and Discovery - Context

**Gathered:** 2026-04-05
**Status:** Ready for planning

<domain>
## Phase Boundary

OSC status feedback to Companion + mDNS advertisement. PtzOsc gains a send path: reply-to-sender using the cached IP:port from the last received OSC packet. Companion receives per-axis moving flags, the active speed preset, and WiFi RSSI as integer-only OSC messages (FB-04) on separate scalar addresses. The device advertises as `ptzhead.local` via `_osc._udp` mDNS so Companion/tools can discover it without a fixed IP. Heartbeat watchdog (500ms) stays untouched and independent — outgoing feedback traffic must not affect `lastRxMs_`. No motion-control behavior changes. Hardware validation remains deferred to Phase 4.

</domain>

<decisions>
## Implementation Decisions

### Feedback address schema (FB-01, FB-02, FB-03, FB-04)
- Separate scalar addresses under `/ptz/status/...`:
  - `/ptz/status/pan/moving i` — 0 or 1
  - `/ptz/status/tilt/moving i` — 0 or 1
  - `/ptz/status/zoom/moving i` — 0 or 1
  - `/ptz/status/preset i` — active preset index (0..kPresetCount-1)
  - `/ptz/status/rssi i` — WiFi RSSI in dBm (negative int, e.g. -62)
- One scalar per address to match Companion generic-osc's one-variable-per-address model.
- All values are integers on the wire (FB-04). No floats, no bundles.

### Send cadence
- Mixed: state-on-change + metrics-periodic.
- **On-change** (emit immediately when value flips):
  - per-axis `moving` flags — derived from `PtzMotion::isMoving()` split per axis (need per-axis query; see Claude's Discretion)
  - `preset` — emit from inside `applySpeedPreset()` acknowledgement path (or detected via change in main loop)
- **Periodic every 1000ms**:
  - `rssi` — read `WiFi.RSSI()` and send
  - Also re-emit a full state snapshot every 1s as a self-heal against packet loss (cheap, 5 ints total).
- Feedback emission happens from the main loop (not ISR); PtzOsc owns a small timer.

### Reply target
- **Reply-to-sender**: `PtzOsc::update()` caches `s_udp.remoteIP()` and `s_udp.remotePort()` from the last successfully parsed packet into `lastSender_` (IPAddress + uint16_t port).
- Feedback sends go to `lastSender_` — the ephemeral source port Companion used for its last TX.
- If no packet has been received yet, feedback is silently suppressed (no broadcast fallback in v1).
- Single Companion sender assumption holds from Phase 2 — no multi-target fanout.

### mDNS advertisement (NET-03)
- Hostname: `ptzhead.local` (locked by requirement).
- Service: `_osc._udp` on port `kOscPort` (8000).
- Service instance name: `PTZHead`.
- **No TXT records** in v1 — minimal surface.
- Lifecycle: start `MDNS.begin("ptzhead")` + `MDNS.addService(...)` on WiFi `SYSTEM_EVENT_STA_GOT_IP`. On reconnect, call `MDNS.end()` then begin again (restart on every GOT_IP event). Piggyback on the existing event-driven WiFi handler from Phase 2.
- Lives in a new small helper (e.g. `src/ptz_mdns.{h,cpp}`) or as part of `ptz_wifi`. Claude's discretion.

### Heartbeat safety invariant (carried forward)
- `PtzOsc::lastRxMs_` stays RX-only. The send path must NOT touch it.
- 500ms heartbeat watchdog is unchanged — its definition of "silent" remains "no OSC received", independent of whether we are transmitting feedback.

### Claude's Discretion
- Per-axis `isMoving()` split: `PtzMotion` currently exposes a single `isMoving()`. Add `isPanMoving()`, `isTiltMoving()`, `isZoomMoving()` (or `axisMoving(AxisId)`) to enable per-axis feedback flags.
- Where feedback lives: extend `PtzOsc` with `sendStatus()` methods vs. a new `PtzFeedback` class. Leaning: extend `PtzOsc` since it already owns `s_udp` and knows the sender.
- Change detection bookkeeping: cache last-sent values in PtzOsc and diff each tick, vs. push notifications from PtzMotion on state transitions. Leaning: cache+diff in PtzOsc (simpler, one owner).
- Whether to coalesce the 1s periodic snapshot with on-change into a single code path that always re-emits everything on a 1s tick plus emits on flip.
- New LogRateIds for feedback TX events (rate-limited to avoid log spam).
- Whether mDNS logic warrants its own module (`ptz_mdns`) or stays inside `ptz_wifi`.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Requirements
- `.planning/REQUIREMENTS.md` — NET-03 (mDNS), FB-01..FB-04 (feedback, integer-only)

### Prior phase (carries forward decisions)
- `.planning/phases/02-network-and-core-osc-control/02-CONTEXT.md` — CNMAT/OSC lib, UDP port 8000, 500ms heartbeat, single-Companion sender assumption, event-driven WiFi reconnect, deferred "cache sender IP for reply-to-sender" item now activated here

### Existing code to modify/extend
- `src/ptz_osc.h` / `src/ptz_osc.cpp` — extend with sender-cache (`lastSender_`), send path, periodic tick, status cache & diff
- `src/ptz_motion.h` / `src/ptz_motion.cpp` — add per-axis `isMoving()` accessors
- `src/ptz_wifi.h` / `src/ptz_wifi.cpp` — hook mDNS start/restart into GOT_IP event (or delegate to new `ptz_mdns`)
- `src/ptz_config.h` — add feedback cadence constant (e.g. `kFeedbackPeriodMs = 1000`), mDNS hostname/service constants, new LogRateIds for feedback TX
- `src/main.cpp` — call `osc.updateFeedback()` (or equivalent) each loop iteration after `osc.update()`

### Research (from Phase 2)
- `.planning/research/PITFALLS.md` — UDP packet-loss watchdog and CNMAT/OSC heap behavior (feedback TX adds heap pressure — monitor)

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `PtzOsc` already owns `WiFiUDP s_udp` — extend, don't add a second UDP socket. WiFiUDP supports `beginPacket(ip,port)` + `write(buf,len)` + `endPacket()` for outbound sends.
- CNMAT `OSCMessage` supports `send(UDP&)` for TX — use same library for outbound, zero new deps.
- `PtzMotion::isMoving()` exists as a single aggregate boolean — needs per-axis split (simple: query each stepper individually).
- `ptz::log` rate limiter reused for feedback-TX log entries with new LogRateIds.
- WiFi event handler from Phase 2 (`SYSTEM_EVENT_STA_DISCONNECTED`) — add parallel `SYSTEM_EVENT_STA_GOT_IP` handler for mDNS start/restart.

### Established Patterns
- `ptz::` namespace, `constexpr` constants in `ptz_config.h`
- File-static trampolines in `ptz_osc.cpp` for CNMAT callbacks — same pattern applies for TX timing bookkeeping (or just use member methods called from main loop).
- Rate-limited logging for repetitive events.

### Integration Points
- `main.cpp` loop order: `osc.update()` (RX drain) → `osc.updateFeedback()` (TX tick) → `motion.update()` (stepper tick) → heartbeat check.
- `platformio.ini` lib_deps: no additions — CNMAT/OSC and ESPmDNS (part of Arduino-ESP32 core) cover everything.
- mDNS: include `<ESPmDNS.h>` (built into espressif32 Arduino framework — no lib_deps entry needed).

### New Additions
- `PtzOsc::lastSender_` (IPAddress + uint16_t port, set in `update()` after successful parse)
- `PtzOsc::updateFeedback()` / `PtzOsc::sendStatusSnapshot()` — iterates cached state, diffs, sends changed values + periodic snapshot
- `PtzOsc::lastFeedbackMs_` — 1s periodic tick tracker
- `PtzOsc::lastSent_` struct — cached previous values for change detection (pan/tilt/zoom moving bits, preset, rssi)
- Optional new module `src/ptz_mdns.{h,cpp}` with `begin(const char* host)` and a restart hook

</code_context>

<specifics>
## Specific Ideas

- Companion generic-osc will bind one variable per address (`/ptz/status/pan/moving`, etc.) and light button LEDs based on the 0/1 value — matches the schema directly.
- RSSI as a raw dBm int (e.g. -62) is what Companion users expect; no bar-graph conversion on-device.
- 1s periodic snapshot doubles as a keepalive — if Companion restarts or drops a packet, all state recovers within 1s.
- `_osc._udp` is the correct mDNS service type for OSC-over-UDP; discovery tools (oscfinder, etc.) recognize it.
- Port `kOscPort` (8000) is advertised — single port for both RX and TX (TX uses ephemeral remote port from sender cache, but listener port stays fixed).

</specifics>

<deferred>
## Deferred Ideas

- TXT records (fw version, axes, active preset) — add if discovery tools or network management need richer metadata.
- Fixed-IP feedback fallback for pre-RX state — only needed if Companion must receive feedback before it sends any command (not currently the flow).
- Broadcast feedback for multi-listener scenarios — out of scope while single-Companion assumption holds.
- Separate outbound port (e.g. 9000) — only if Companion generic-osc ever needs to listen on a fixed non-ephemeral port.
- Floats on the wire — explicitly rejected by FB-04.
- Multi-target fanout (several Companions) — defer until requested.

</deferred>

---

*Phase: 03-feedback-and-discovery*
*Context gathered: 2026-04-05*
