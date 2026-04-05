---
phase: 2
slug: network-and-core-osc-control
status: draft
nyquist_compliant: false
wave_0_complete: false
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
| TBD | TBD | TBD | TBD | compile | `pio run -e esp32dev` | ❌ W0 | ⬜ pending |

*Populated once plans are authored. Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] Add CNMAT/OSC@^3.5.8 to platformio.ini `lib_deps`
- [ ] Confirm `pio run` succeeds with new dependency

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

- [ ] All tasks have `<automated>` compile verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without compile verify
- [ ] Wave 0 covers CNMAT/OSC dependency install
- [ ] No watch-mode flags
- [ ] Feedback latency < 60s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
