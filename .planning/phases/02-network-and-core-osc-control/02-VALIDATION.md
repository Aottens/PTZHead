---
phase: 2
slug: network-and-core-osc-control
status: draft
nyquist_compliant: true
wave_0_complete: true
created: 2026-04-05
---

# Phase 2 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | PlatformIO native build + runtime serial logs |
| **Config file** | platformio.ini |
| **Quick run command** | `pio run -e esp32dev` |
| **Full suite command** | `pio run -e esp32dev && pio device monitor -e esp32dev` |
| **Estimated runtime** | ~30 seconds compile |

---

## Sampling Rate

- **After every task commit:** Run `pio run -e esp32dev` (compile check)
- **After every plan wave:** Full compile + flash + serial log capture
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** 60 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 02-01-T1 | 02-01 | 1 | NET-04 (foundation), SPD-01/02/03 scaffolding, MOT-07 scaffolding | grep + compile | `grep -E "kOscPort\|kHeartbeatTimeoutMs\|kSpeedPresets\|kDefaultPresetIndex\|kPresetCount\|kLogRateOscRxVelocity\|kLogRateHeartbeatFired\|kLogRateCount = 7" src/ptz_config.h && pio run 2>&1 \| tail -5` | ✅ | ⬜ pending |
| 02-01-T2 | 02-01 | 1 | NET-01, NET-02, NET-05 | grep + compile | `grep -c "WIFI_PS_NONE" src/ptz_wifi.cpp && grep -c "WIFI_PS_MIN_MODEM" src/ptz_wifi.cpp && grep "ARDUINO_EVENT_WIFI_STA_DISCONNECTED" src/ptz_wifi.cpp && grep "ARDUINO_EVENT_WIFI_STA_GOT_IP" src/ptz_wifi.cpp && grep "WiFi.reconnect()" src/ptz_wifi.cpp && pio run 2>&1 \| tail -5` | ✅ | ⬜ pending |
| 02-02-T1 | 02-02 | 1 | SPD-01, SPD-02, SPD-03, MOT-06 (header) | grep (header) | `grep -E "setPanVelocity\|setTiltVelocity\|setZoomVelocity\|stopPan\|stopTilt\|stopZoom\|applySpeedPreset\|activePreset" src/ptz_motion.h \| wc -l \| awk '$1 >= 8 {exit 0} {exit 1}'` | ✅ | ⬜ pending |
| 02-02-T2 | 02-02 | 1 | MOT-06, SPD-01, SPD-02, SPD-03 | grep + compile | `grep -E "setPanVelocity\|setTiltVelocity\|setZoomVelocity\|stopPan\|stopTilt\|stopZoom\|applySpeedPreset" src/ptz_motion.cpp \| wc -l \| awk '$1 >= 7 {exit 0} {exit 1}' && grep "applySpeedPreset(kDefaultPresetIndex)" src/ptz_motion.cpp && grep "panMaxSpsEff_" src/ptz_motion.cpp && pio run 2>&1 \| tail -10` | ✅ | ⬜ pending |
| 02-03-T1 | 02-03 | 2 | NET-04 (lib dep) | grep | `grep "cnmat/OSC@\^3.5.8" platformio.ini && grep "gin66/FastAccelStepper@\^0.31.0" platformio.ini && grep "tzapu/WiFiManager@\^2.0.17" platformio.ini && grep "platform = espressif32@6.10.0" platformio.ini` | ✅ | ⬜ pending |
| 02-03-T2 | 02-03 | 2 | MOT-01, MOT-02, MOT-03, MOT-04, MOT-05, NET-04 | grep + compile | `test -f src/ptz_osc.h && test -f src/ptz_osc.cpp && grep "class PtzOsc" src/ptz_osc.h && grep "motionPtr()" src/ptz_osc.h && grep "while ((size = s_udp.parsePacket()) > 0)" src/ptz_osc.cpp && grep 'msg.dispatch("/ptz/pan",' src/ptz_osc.cpp && grep 'msg.dispatch("/ptz/pan/stop",' src/ptz_osc.cpp && grep 'msg.dispatch("/ptz/tilt",' src/ptz_osc.cpp && grep 'msg.dispatch("/ptz/tilt/stop",' src/ptz_osc.cpp && grep 'msg.dispatch("/ptz/zoom",' src/ptz_osc.cpp && grep 'msg.dispatch("/ptz/zoom/stop",' src/ptz_osc.cpp && grep 'msg.dispatch("/ptz/stop",' src/ptz_osc.cpp && grep 'msg.dispatch("/ptz/speed/preset",' src/ptz_osc.cpp && grep "s_udp.begin(kOscPort)" src/ptz_osc.cpp && pio run 2>&1 \| tail -15` | ✅ | ⬜ pending |
| 02-03-T3 | 02-03 | 2 | MOT-07, NET-04 | grep + compile | `grep '#include "ptz_osc.h"' src/main.cpp && grep "ptz::PtzOsc g_osc;" src/main.cpp && grep "g_osc.begin(&g_motion)" src/main.cpp && grep "g_osc.update()" src/main.cpp && grep "kHeartbeatTimeoutMs" src/main.cpp && grep "g_motion.stop()" src/main.cpp && grep "ESP.getFreeHeap()" src/main.cpp && grep "handleSerialCommands()" src/main.cpp && pio run 2>&1 \| tail -15` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [x] Add CNMAT/OSC@^3.5.8 to platformio.ini `lib_deps` (covered by 02-03-T1)
- [x] Every task has an `<automated>` verify command

*Manual (hardware loop) verification supplements compile-time checks — see below.*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| OSC /ptz/pan drives motor at commanded velocity | MOT-01, MOT-02 | Requires hardware + OSC sender | Send `/ptz/pan 0.5` from TouchOSC/Companion, observe pan motor |
| /ptz/stop halts all axes smoothly | MOT-04 | Requires hardware | Hold pan, send `/ptz/stop`, observe deceleration |
| /ptz/speed/preset switches profile live | SPD-01, SPD-02, SPD-03 | Requires hardware + OSC sender | Switch preset mid-movement, observe speed change |
| WiFi auto-reconnect | NET-02, NET-03 | Requires router reboot | Power-cycle router, verify reconnect + resumed OSC handling |
| Heartbeat timeout motor auto-stop | NET-05, MOT-07 | Requires OSC command silence | Stop sending OSC, verify motors stop within timeout window |
| OSC-to-motor latency < 50ms | NET-04 | Requires measurement tooling | Serial-log timestamps at parse and setAxisVelocity entry |
| WiFi power save disabled | NET-04 | Runtime verification | Serial log confirms `WiFi.setSleep(false)` on connect + GOT_IP |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` compile verify or Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without compile verify
- [x] Wave 0 covers CNMAT/OSC dependency install
- [x] No watch-mode flags
- [x] Feedback latency < 60s
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
