---
phase: 03-feedback-and-discovery
verified: 2026-04-05T19:10:00Z
status: passed
score: 9/9 must-haves verified
---

# Phase 3: Feedback and Discovery Verification Report

**Phase Goal:** Companion receives live status from the PTZ head (moving state, speed preset, signal strength) and the device is discoverable via mDNS
**Verified:** 2026-04-05T19:10:00Z
**Status:** passed
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| #  | Truth | Status | Evidence |
|----|-------|--------|----------|
| 1  | Companion receives per-axis moving state as integer values (0/1) | VERIFIED | `updateFeedback()` reads `isPanMoving/isTiltMoving/isZoomMoving` and calls `sendScalarInt` with `?1:0` cast; `msg.add(value)` produces OSC 'i' type tag |
| 2  | Companion receives active speed preset ID and WiFi RSSI as integer values | VERIFIED | `cur.preset = static_cast<int32_t>(motion_->activePreset())` and `cur.rssi = static_cast<int32_t>(WiFi.RSSI())` both sent via `sendScalarInt` in the 1s periodic snapshot |
| 3  | All feedback values are integers (no floats) — Companion generic-osc compatible | VERIFIED | `StatusSnapshot` fields are all `int32_t`; `sendScalarInt` calls `msg.add(int32_t)` exclusively; no float type tag used anywhere in TX path |
| 4  | Device advertises as `ptzhead.local` with `_osc._udp` service type and is resolvable | VERIFIED | `MDNS.begin(kMdnsHostname)` / `MDNS.addService(kMdnsServiceType, kMdnsServiceProto, kOscPort)` / `MDNS.setInstanceName(kMdnsInstanceName)` all present in GOT_IP handler in `ptz_wifi.cpp`; `kMdnsHostname="ptzhead"`, service `_osc._udp:8000` |
| 5  | PtzMotion exposes per-axis isMoving queries that return stepper isRunning state | VERIFIED | `isPanMoving/isTiltMoving/isZoomMoving` declared in `ptz_motion.h` (lines 13-15), implemented in `ptz_motion.cpp` (lines 122-124) with null-guard and `isRunning()` delegation |
| 6  | updateFeedback() is wired into main loop after osc.update() | VERIFIED | `main.cpp` loop lines 69-70: `g_osc.update()` immediately followed by `g_osc.updateFeedback()` |
| 7  | Feedback is silently suppressed before first RX and when WiFi disconnected | VERIFIED | `if (!hasSender()) return` at top of `updateFeedback()` (port=0 sentinel); `if (WiFi.status() != WL_CONNECTED) return` before RSSI read |
| 8  | Heartbeat invariant preserved — lastRxMs_ written only from RX path | VERIFIED | Two write sites confirmed: `begin()` seed (line 106) and `update()` parse-success (line 125); TX path (`sendScalarInt`, `updateFeedback`) contains no `lastRxMs_` writes |
| 9  | mDNS restarts on every GOT_IP event (reconnect resilience) | VERIFIED | `MDNS.end()` unconditionally before `MDNS.begin(kMdnsHostname)` inside the `ARDUINO_EVENT_WIFI_STA_GOT_IP` handler |

**Score:** 9/9 truths verified

---

### Required Artifacts

#### Plan 01 Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/ptz_motion.h` | Per-axis isMoving const accessors | VERIFIED | Lines 12-15: `isMoving/isPanMoving/isTiltMoving/isZoomMoving` all declared `const` |
| `src/ptz_motion.cpp` | isRunning per-axis delegating implementations | VERIFIED | Lines 122-128: three per-axis bodies + aggregate OR; null-guards present |
| `src/ptz_config.h` | kFeedbackPeriodMs | VERIFIED | Line 35: `constexpr uint32_t kFeedbackPeriodMs = 1000` |
| `src/ptz_config.h` | kMdnsHostname | VERIFIED | Line 45: `constexpr const char* kMdnsHostname = "ptzhead"` |
| `src/ptz_config.h` | kLogRateFeedbackTx | VERIFIED | Line 96: `kLogRateFeedbackTx = 7` |
| `src/ptz_config.h` | kLogRateMdns + kLogRateCount=9 | VERIFIED | Lines 97-98: `kLogRateMdns = 8`, `kLogRateCount = 9` |
| `src/ptz_config.h` | 5 feedback OSC address constants | VERIFIED | Lines 38-42: all five `kOscAddr*` constants present |
| `src/ptz_config.h` | 4 mDNS constants | VERIFIED | Lines 45-48: hostname, instance name, service type, service proto all present |

#### Plan 02 Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/ptz_osc.h` | StatusSnapshot struct, updateFeedback() API, sender cache | VERIFIED | Lines 9-15: `StatusSnapshot` struct with `preset=-1` sentinel; lines 30/33: `updateFeedback()` + `hasSender()`; lines 48-53: `lastSenderIp_`, `lastSenderPort_`, `lastFeedbackMs_`, `lastSent_`, `sendScalarInt` |
| `src/ptz_osc.cpp` | Sender capture before buffer drain | VERIFIED | Lines 114-115: `remoteIP()` / `remotePort()` captured before `msg.fill()` loop |
| `src/ptz_osc.cpp` | sendScalarInt with heap hygiene | VERIFIED | Lines 132-140: `beginPacket` / `msg.send` / `endPacket` / `msg.empty()` — all four steps present |
| `src/ptz_osc.cpp` | updateFeedback diff + periodic loop | VERIFIED | Lines 142-178: on-change diff (lines 158-161), periodic tick (lines 164-175), `lastSent_ = cur` (line 177) |
| `src/main.cpp` | updateFeedback call in loop after update() | VERIFIED | Line 70: `g_osc.updateFeedback()` immediately after `g_osc.update()` on line 69 |

#### Plan 03 Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/ptz_wifi.cpp` | `#include <ESPmDNS.h>` | VERIFIED | Line 6: present |
| `src/ptz_wifi.cpp` | MDNS.begin | VERIFIED | Line 42: `MDNS.begin(kMdnsHostname)` |
| `src/ptz_wifi.cpp` | MDNS.end() | VERIFIED | Line 41: `MDNS.end()` before `MDNS.begin()` |
| `src/ptz_wifi.cpp` | MDNS.addService | VERIFIED | Line 44: `MDNS.addService(kMdnsServiceType, kMdnsServiceProto, kOscPort)` |
| `src/ptz_wifi.cpp` | MDNS.setInstanceName | VERIFIED | Line 43: `MDNS.setInstanceName(kMdnsInstanceName)` |

---

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `ptz_motion.h isPanMoving` | `FastAccelStepper::isRunning()` | null-guard + isRunning call | WIRED | `ptz_motion.cpp:122`: `pan_ && pan_->isRunning()` |
| `isMoving() aggregate` | per-axis accessors | logical OR | WIRED | `ptz_motion.cpp:126-128`: `isPanMoving() \|\| isTiltMoving() \|\| isZoomMoving()` |
| `PtzOsc::update() parse-success` | `lastSenderIp_ / lastSenderPort_` | remoteIP()/remotePort() before buffer drain | WIRED | `ptz_osc.cpp:114-127`: capture before `msg.fill()` loop, assigned after `hasError()` check |
| `PtzOsc::updateFeedback()` | PtzMotion per-axis accessors | `motion_->isPanMoving/isTiltMoving/isZoomMoving/activePreset` | WIRED | `ptz_osc.cpp:151-154`: all four accessor calls present |
| `PtzOsc::updateFeedback()` | lastSender_ cached tuple | `hasSender()` gate + `sendScalarInt -> beginPacket(lastSenderIp_, lastSenderPort_)` | WIRED | `ptz_osc.cpp:143` gate; `ptz_osc.cpp:136` send target |
| `main.cpp loop()` | `g_osc.updateFeedback()` | called immediately after `g_osc.update()` every loop tick | WIRED | `main.cpp:69-70`: sequential, no logic between them |
| `WiFi GOT_IP event handler` | ESPmDNS responder | `MDNS.end()` then `MDNS.begin(kMdnsHostname)` then `setInstanceName + addService` | WIRED | `ptz_wifi.cpp:41-44`: complete mDNS lifecycle inside single GOT_IP lambda |
| `MDNS.addService` | kOscPort + kMdnsServiceType + kMdnsServiceProto | service advertisement on port 8000 as `_osc._udp` | WIRED | `ptz_wifi.cpp:44`: `MDNS.addService(kMdnsServiceType, kMdnsServiceProto, kOscPort)` |

---

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|-------------|-------------|--------|----------|
| NET-03 | 03-01-PLAN, 03-03-PLAN | Firmware advertises via mDNS as `ptzhead.local` with `_osc._udp` service | SATISFIED | `ptz_wifi.cpp`: `MDNS.begin("ptzhead")` + `addService("_osc","_udp",8000)` + `setInstanceName("PTZHead")` inside GOT_IP handler; constants from `ptz_config.h` |
| FB-01 | 03-01-PLAN, 03-02-PLAN | Firmware sends per-axis moving state back via OSC (integer: 0/1) | SATISFIED | `updateFeedback()` reads `isPanMoving/isTiltMoving/isZoomMoving`, casts to `int32_t` (0 or 1), emits on `/ptz/status/{pan,tilt,zoom}/moving` via `sendScalarInt` |
| FB-02 | 03-02-PLAN | Firmware sends active speed preset ID back via OSC (integer) | SATISFIED | `cur.preset = static_cast<int32_t>(motion_->activePreset())` emitted on `/ptz/status/preset` on-change and each 1s periodic snapshot |
| FB-03 | 03-02-PLAN | Firmware sends WiFi RSSI back via OSC (integer) | SATISFIED | `cur.rssi = static_cast<int32_t>(WiFi.RSSI())` emitted on `/ptz/status/rssi` each 1s periodic snapshot; gated on `WL_CONNECTED` |
| FB-04 | 03-01-PLAN, 03-02-PLAN | All OSC feedback uses integer values (Companion generic-osc compatibility) | SATISFIED | `StatusSnapshot` fields are all `int32_t`; `sendScalarInt` calls `msg.add(int32_t)` which produces OSC 'i' type tag; no float used in TX path |

All 5 requirements declared across the 3 plans are accounted for. No orphaned requirements found.

---

### Anti-Patterns Found

No anti-patterns detected. Scan of all 7 modified files (`ptz_motion.h`, `ptz_motion.cpp`, `ptz_config.h`, `ptz_osc.h`, `ptz_osc.cpp`, `ptz_wifi.cpp`, `main.cpp`) produced zero matches for TODO/FIXME/placeholder/stub patterns. No empty implementations, no console-only handlers.

---

### Human Verification Required

The following items cannot be verified programmatically and require hardware or live-network testing in Phase 4:

#### 1. mDNS First-Boot Advertisement

**Test:** Flash firmware to ESP32. On first boot (stored WiFi credentials present), monitor serial output; then from another device on the same network, run `ping ptzhead.local` or `dns-sd -B _osc._udp local.`
**Expected:** `ptzhead.local` resolves to the ESP32 IP; Bonjour browser lists `PTZHead` instance advertising `_osc._udp:8000`
**Why human:** The plan explicitly notes that the first GOT_IP event on boot may fire before `registerWifiEventHandlers()` is registered in `tryStoredCredentials()`. The code path is correct for reconnects but the first-boot timing gap requires hardware observation to confirm whether mDNS starts or requires one disconnect/reconnect cycle.

#### 2. Feedback Round-Trip Latency

**Test:** Send a `/ptz/pan 1.0` OSC command from Companion. Observe the `/ptz/status/pan/moving` feedback message arriving at the Companion OSC listener.
**Expected:** Within one loop tick (sub-millisecond after the next `g_osc.update()` call), Companion receives `1` on `/ptz/status/pan/moving`
**Why human:** On-change diff correctness and loop timing cannot be verified without live hardware running the actual loop() cadence.

#### 3. RSSI Reporting

**Test:** While device is connected to WiFi, observe `/ptz/status/rssi` arriving at the Companion OSC sender every ~1000ms.
**Expected:** Non-zero negative integer (e.g., `-65`) reflecting actual signal strength; value updates on each 1s periodic snapshot
**Why human:** `WiFi.RSSI()` return value validity on ESP32 requires live WiFi association to confirm.

---

### Commit Verification

All commits documented in SUMMARY files are confirmed present in git log:

| Commit | Plan | Description |
|--------|------|-------------|
| `5b150e1` | 03-01 | feat(03-01): add per-axis isMoving accessors to PtzMotion |
| `750bcd4` | 03-01 | feat(03-01): add Phase 3 feedback, mDNS, and LogRateId constants |
| `bcadda2` | 03-02 | feat(03-02): extend PtzOsc header with feedback TX interface |
| `4173da1` | 03-02 | feat(03-02): implement OSC feedback TX path in PtzOsc |
| `ce1b90d` | 03-02 | feat(03-02): wire updateFeedback() into main loop |
| `8c38fa9` | 03-03 | feat(03-03): add mDNS advertisement on GOT_IP event |

---

### Summary

Phase 3 goal is fully achieved at the code level. All three plans produced substantive, fully wired implementations:

- **Plan 01:** Foundation constants and per-axis motion accessors are in place — exactly the right shapes for Plans 02 and 03 to consume without literals.
- **Plan 02:** The complete OSC feedback TX path is implemented: sender cache (captured before buffer drain per Pitfall 5), `sendScalarInt` with `msg.empty()` heap hygiene, on-change diff + 1s self-heal snapshot, heartbeat invariant preserved (`lastRxMs_` written exactly twice — both from the RX path only).
- **Plan 03:** mDNS lifecycle is correctly bound to `ARDUINO_EVENT_WIFI_STA_GOT_IP` with `MDNS.end()` before `MDNS.begin()` for reconnect resilience; all four service advertisement calls present using Plan 01 constants.

One known uncertainty (first-boot mDNS timing gap) is documented in the plan itself and flagged for Phase 4 hardware validation. It is not a code defect — the logic is correct for reconnects; only the first-boot ordering edge case requires runtime observation.

---

_Verified: 2026-04-05T19:10:00Z_
_Verifier: Claude (gsd-verifier)_
