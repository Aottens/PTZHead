---
phase: 1
slug: platform-migration-and-cleanup
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-04-04
---

# Phase 1 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | PlatformIO build + manual serial verification |
| **Config file** | `platformio.ini` |
| **Quick run command** | `pio run` |
| **Full suite command** | `pio run --target upload && pio device monitor` |
| **Estimated runtime** | ~30 seconds (compile), manual testing on hardware |

---

## Sampling Rate

- **After every task commit:** Run `pio run` (compile check)
- **After every plan wave:** Run `pio run --target upload && pio device monitor` (hardware verify)
- **Before `/gsd:verify-work`:** Full suite must compile and boot
- **Max feedback latency:** 30 seconds (compile)

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 01-01-01 | 01 | 1 | PLAT-01 | build | `pio run` | N/A | ⬜ pending |
| 01-01-02 | 01 | 1 | PLAT-02 | build+grep | `pio run && ! grep -r "Bluepad32\|bluepad32" src/` | N/A | ⬜ pending |
| 01-01-03 | 01 | 1 | PLAT-03 | build+grep | `pio run && ! grep -r "WebSocket\|ptz_ws" src/` | N/A | ⬜ pending |
| 01-01-04 | 01 | 1 | PLAT-04 | build+grep | `pio run && ! grep -r "ptz_owner\|Owner" src/` | N/A | ⬜ pending |
| 01-01-05 | 01 | 1 | PLAT-05 | build+manual | `pio run` + serial motor test | N/A | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

Existing infrastructure covers all phase requirements. PlatformIO build system is already configured. No test framework needed — this is a hardware firmware project validated by compile + hardware test.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| WiFi connects after platform switch | PLAT-01 | Requires physical ESP32 + WiFi AP | Flash firmware, open serial monitor, verify WiFiManager connects or starts captive portal |
| Motors respond to serial commands | PLAT-05 | Requires physical stepper drivers | Type 'PAN 0.5' in serial monitor, verify pan motor moves. Test all 3 axes + STOP |
| FastAccelStepper acceleration feels smooth | PLAT-05 | Subjective hardware verification | Run motors at various speeds, verify no stuttering or jerky motion |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 30s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
