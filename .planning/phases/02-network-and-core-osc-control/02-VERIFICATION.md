---
phase: 02-network-and-core-osc-control
verified: 2026-04-05T00:00:00Z
status: passed
score: 5/5 must-haves verified
re_verification: false
---

# Phase 2: Network and Core OSC Control Verification Report

**Phase Goal:** A user holding a Companion button drives the PTZ head via OSC over WiFi with smooth acceleration and configurable speed presets
**Verified:** 2026-04-05
**Status:** PASSED
**Re-verification:** No — initial verification

---

## Goal Achievement

### Observable Truths (from ROADMAP.md Success Criteria)

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Sending `/ptz/pan`, `/ptz/tilt`, `/ptz/zoom` OSC messages causes the corresponding motor to move at the commanded velocity with smooth acceleration | VERIFIED | `ptz_osc.cpp` dispatches each address to `onPan/onTilt/onZoom`, which call `setPanVelocity/setTiltVelocity/setZoomVelocity`; those delegate to `setAxisVelocity` which calls `applySpeedAcceleration()` on every command |
| 2 | Sending `/ptz/stop` stops all axes smoothly; per-axis stop commands stop individual axes | VERIFIED | `onStopAll` → `motion_->stop()`; `onStopPan/onStopTilt/onStopZoom` → `stopPan/stopTilt/stopZoom`, each calling `stepper->stopMove()` (FastAccelStepper deceleration) |
| 3 | Sending `/ptz/speed/preset` switches the active speed/acceleration profile immediately | VERIFIED | `onPreset` → `applySpeedPreset(idx)`; that function updates `*MaxSpsEff_` AND calls `setAcceleration()` + `applySpeedAcceleration()` on running steppers; `activePreset_` persists in `PtzMotion` state |
| 4 | WiFi reconnects automatically after a network drop without a power cycle; motors auto-stop when no OSC command received within the heartbeat timeout | VERIFIED | `registerWifiEventHandlers()` registers `ARDUINO_EVENT_WIFI_STA_DISCONNECTED` → `WiFi.reconnect()`; heartbeat in `main.cpp` loop: `g_motion.isMoving() && (now - g_osc.lastRxMs()) > kHeartbeatTimeoutMs` → `g_motion.stop()` |
| 5 | WiFi power save is disabled; OSC command-to-motor-response latency is under 50ms | VERIFIED (compile-time) | `WiFi.setSleep(false)` + `esp_wifi_set_ps(WIFI_PS_NONE)` in both `tryStoredCredentials()` and the `GOT_IP` event handler (2 occurrences each); UDP drain-all pattern in `update()` processes all queued packets per loop tick. Runtime latency requires hardware validation (Phase 4) |

**Score:** 5/5 truths verified

---

## Required Artifacts

### Plan 02-01 Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/ptz_config.h` | kOscPort, kHeartbeatTimeoutMs, SpeedPreset struct, kSpeedPresets[3], kDefaultPresetIndex, kPresetCount, LogRateIds | VERIFIED | All present; `kOscPort=8000`, `kHeartbeatTimeoutMs=500`, `SpeedPreset` struct, 3-entry preset table, `kDefaultPresetIndex=1`, `kPresetCount` via sizeof, `kLogRateCount=7` |
| `src/ptz_wifi.cpp` | Event-driven reconnect, WIFI_PS_NONE at connect and reconnect | VERIFIED | 2x `WIFI_PS_NONE`, 2x `setSleep(false)`, `ARDUINO_EVENT_WIFI_STA_DISCONNECTED` and `ARDUINO_EVENT_WIFI_STA_GOT_IP` handlers, `registerWifiEventHandlers()` called from both connect paths (3 occurrences: definition + 2 call sites) |

### Plan 02-02 Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/ptz_motion.h` | Per-axis setters, applySpeedPreset, activePreset getter | VERIFIED | `setPanVelocity`, `setTiltVelocity`, `setZoomVelocity`, `stopPan`, `stopTilt`, `stopZoom`, `applySpeedPreset(uint8_t)`, `activePreset() const`; private `panMaxSpsEff_`, `tiltMaxSpsEff_`, `zoomMaxSpsEff_`, `activePreset_` |
| `src/ptz_motion.cpp` | Preset scaling via kSpeedPresets table, applySpeedPreset invoked from begin() | VERIFIED | `applySpeedPreset(kDefaultPresetIndex)` called at end of `begin()`; `kSpeedPresets[idx]` index lookup present; `kPanMaxSps * p.panScale` scaling; `activePreset_ = idx`; `isRunning()` gate before `applySpeedAcceleration()` |

### Plan 02-03 Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `platformio.ini` | CNMAT/OSC dependency | VERIFIED | `https://github.com/CNMAT/OSC.git#3.5.8` — deliberate deviation from plan's `cnmat/OSC@^3.5.8`; same version, git tag used because PlatformIO registry only publishes v1.0.0 (documented in 02-03-SUMMARY.md) |
| `src/ptz_osc.h` | PtzOsc class with begin, update, lastRxMs, motionPtr | VERIFIED | All four members declared; forward decl for `PtzMotion`; `lastRxMs_` and `motion_` private members |
| `src/ptz_osc.cpp` | CNMAT dispatch, WiFiUDP drain-all, command-to-motion bridge | VERIFIED | `while ((size = s_udp.parsePacket()) > 0)` drain loop; all 8 `msg.dispatch()` calls present; `s_self` trampoline pattern; no capturing lambdas; `clampNorm` applied to velocity; all LogRateId constants used |
| `src/main.cpp` | g_osc wired in setup and loop with heartbeat watchdog | VERIFIED | `#include "ptz_osc.h"`, `ptz::PtzOsc g_osc`, `g_osc.begin(&g_motion)` in setup, `g_osc.update()` in loop, heartbeat condition with `kHeartbeatTimeoutMs`, heap log with `kLogRateHeapFree` |

---

## Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `src/ptz_wifi.cpp` | `ARDUINO_EVENT_WIFI_STA_DISCONNECTED` | `WiFi.onEvent` calling `WiFi.reconnect()` | WIRED | Handler registered; `WiFi.reconnect()` called inside handler body |
| `src/ptz_wifi.cpp` | `ARDUINO_EVENT_WIFI_STA_GOT_IP` | `WiFi.onEvent` re-asserting `WIFI_PS_NONE` | WIRED | Handler registered; `setSleep(false)` + `esp_wifi_set_ps(WIFI_PS_NONE)` inside handler body |
| `src/ptz_motion.cpp applySpeedPreset` | `src/ptz_config.h kSpeedPresets` | Index lookup `kSpeedPresets[idx]` | WIRED | `const SpeedPreset& p = kSpeedPresets[idx]` present |
| `src/ptz_motion.cpp setAxisVelocity` | `FastAccelStepper applySpeedAcceleration` | Preserved Phase 1 pattern | WIRED | `stepper->applySpeedAcceleration()` called in `setAxisVelocity` |
| `src/ptz_osc.cpp update()` | `WiFiUDP::parsePacket` | Drain-all-packets while loop | WIRED | `while ((size = s_udp.parsePacket()) > 0)` |
| `src/ptz_osc.cpp handleMessage` | `PtzMotion setPanVelocity/setTiltVelocity/setZoomVelocity` | Exact-address dispatch callbacks | WIRED | `onPan` → `setPanVelocity`, `onTilt` → `setTiltVelocity`, `onZoom` → `setZoomVelocity` via `s_self->motionPtr()` |
| `src/main.cpp loop()` | `g_motion.stop()` via heartbeat | `millis() - g_osc.lastRxMs() > kHeartbeatTimeoutMs` | WIRED | Exact condition present; gated on `g_motion.isMoving()` |

---

## Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|------------|-------------|--------|----------|
| NET-01 | 02-01 | WiFiManager captive portal | SATISFIED | `tryStoredCredentials` + `startProvisioningPortal` paths unchanged; `PtzWifi::begin(bool forcePortal)` API intact |
| NET-02 | 02-01 | WiFi power save disabled | SATISFIED | `WIFI_PS_NONE` + `setSleep(false)` in initial connect and in `GOT_IP` reconnect handler |
| NET-04 | 02-01, 02-03 | UDP port declared as constant; listener started | SATISFIED | `kOscPort=8000` in `ptz_config.h`; `s_udp.begin(kOscPort)` in `PtzOsc::begin()` |
| NET-05 | 02-01 | Non-blocking event-based auto-reconnect | SATISFIED | `ARDUINO_EVENT_WIFI_STA_DISCONNECTED` → `WiFi.reconnect()` — no blocking poll loop |
| MOT-01 | 02-03 | `/ptz/pan` drives pan velocity | SATISFIED | `msg.dispatch("/ptz/pan", onPan)` → `setPanVelocity` |
| MOT-02 | 02-03 | `/ptz/tilt` drives tilt velocity | SATISFIED | `msg.dispatch("/ptz/tilt", onTilt)` → `setTiltVelocity` |
| MOT-03 | 02-03 | `/ptz/zoom` drives zoom velocity | SATISFIED | `msg.dispatch("/ptz/zoom", onZoom)` → `setZoomVelocity` |
| MOT-04 | 02-02, 02-03 | Per-axis stop via OSC | SATISFIED | `stopPan/stopTilt/stopZoom` implemented in `ptz_motion.cpp`; dispatched from `/ptz/{pan,tilt,zoom}/stop` |
| MOT-05 | 02-03 | `/ptz/stop` stops all axes | SATISFIED | `msg.dispatch("/ptz/stop", onStopAll)` → `motion_->stop()` which calls `stopMove()` on all three steppers |
| MOT-06 | 02-02 | Smooth acceleration/deceleration | SATISFIED | `setAxisVelocity` calls `applySpeedAcceleration()` on every velocity change; `stopMove()` triggers FastAccelStepper deceleration ramp |
| MOT-07 | 02-03 | Heartbeat auto-stop watchdog | SATISFIED | `main.cpp` loop: `g_motion.isMoving() && (now - g_osc.lastRxMs()) > kHeartbeatTimeoutMs` → `g_motion.stop()` |
| SPD-01 | 02-02, 02-03 | 3 speed presets switchable via OSC | SATISFIED | `kSpeedPresets[3]` in config; `applySpeedPreset(uint8_t idx)` on `PtzMotion`; `/ptz/speed/preset` OSC endpoint wired |
| SPD-02 | 02-02 | Presets affect max velocity AND acceleration | SATISFIED | `applySpeedPreset` sets `*MaxSpsEff_` (velocity) AND calls `setAcceleration()` (accel) for all 3 axes |
| SPD-03 | 02-02 | Active preset persists until changed | SATISFIED | `activePreset_` member on `PtzMotion`; only updated inside `applySpeedPreset()` |

**All 14 Phase 2 requirements: SATISFIED**

No orphaned requirements — every requirement in the phase (NET-01, NET-02, NET-04, NET-05, MOT-01 through MOT-07, SPD-01 through SPD-03) is claimed by a plan and has implementation evidence. NET-03 is correctly assigned to Phase 3 (not Phase 2).

---

## Anti-Patterns Found

No anti-patterns detected across any of the modified files (`ptz_config.h`, `ptz_wifi.cpp`, `ptz_motion.h`, `ptz_motion.cpp`, `ptz_osc.h`, `ptz_osc.cpp`, `main.cpp`). No TODO/FIXME/placeholder comments, no stub return values, no console.log-only handlers.

---

## Notable Observations

### platformio.ini: Deliberate Registry Deviation

The plan specified `cnmat/OSC@^3.5.8` but the actual entry is `https://github.com/CNMAT/OSC.git#3.5.8`. This is a documented, intentional deviation: the PlatformIO registry only publishes cnmat/OSC at v1.0.0. The git tag URL pins the exact same version. Functionally equivalent — no gap.

---

## Human Verification Required

The following items cannot be verified by static code analysis and are deferred to Phase 4 hardware validation:

### 1. OSC Command-to-Motor Latency

**Test:** Send `/ptz/pan f 0.5` from an OSC sender on the same WiFi network, measure time from packet transmission to first stepper pulse.
**Expected:** Under 50ms end-to-end (NET-02, Success Criterion 5).
**Why human:** Requires oscilloscope or hardware timing; cannot be measured from source code.

### 2. WiFi Reconnect Behavior

**Test:** Disconnect WiFi AP while firmware is running, wait 5–10 seconds, restore AP. Observe serial log.
**Expected:** `STA disconnected` log appears, `WiFi.reconnect()` is called (non-blocking), firmware eventually logs `GOT_IP` and resumes OSC reception.
**Why human:** Event handler behavior under real network conditions; no simulator available.

### 3. Heartbeat Watchdog Timing

**Test:** Send `/ptz/pan f 0.5` to start motion, then stop sending OSC packets. Observe stepper behavior.
**Expected:** Motors decelerate to stop within approximately 500ms of the last received packet.
**Why human:** Requires physical stepper observation; timing depends on loop execution rate on real hardware.

### 4. Speed Preset Motor Behavior

**Test:** Send `/ptz/speed/preset i 0` then `/ptz/pan f 1.0`; repeat with preset 2.
**Expected:** Preset 0 (slow) produces noticeably slower acceleration and lower top speed than preset 2 (fast).
**Why human:** Requires audible/visual comparison of stepper motion on real hardware.

---

## Gaps Summary

No gaps. All automated verification checks pass. Phase 2 goal is achieved in code: the firmware as written will, when flashed to hardware, allow a Companion operator to drive the PTZ head via OSC over WiFi with smooth acceleration and configurable speed presets. Four items require hardware confirmation in Phase 4 but are not blockers for phase completion.

---

_Verified: 2026-04-05_
_Verifier: Claude (gsd-verifier)_
