---
phase: 4
slug: end-to-end-hardware-validation
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-04-06
---

# Phase 4 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Manual hardware test checklist (no automated unit tests) |
| **Config file** | None — checklist is in PLAN.md |
| **Quick run command** | `pio run --target upload && pio device monitor` |
| **Full suite command** | Complete test checklist walkthrough (human-driven) |
| **Estimated runtime** | ~30 minutes (full hardware test pass) |

---

## Sampling Rate

- **After every task commit:** Run `pio run` (compile check only — actual validation is human-driven)
- **After every plan wave:** Full test checklist re-run
- **Before `/gsd:verify-work`:** All 7 success criteria must pass
- **Max feedback latency:** N/A (human-observed)

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 04-01-01 | 01 | 1 | PLAT-05 | manual-only | Visual inspection of main.cpp + boot log | N/A | ⬜ pending |
| 04-01-02 | 01 | 1 | MOT-01..03 | manual | Serial: `PAN 1.0`, `TILT 1.0`, `ZOOM 1.0` | N/A | ⬜ pending |
| 04-01-03 | 01 | 1 | MOT-04..05 | manual | Serial: `STOP`, per-axis stop | N/A | ⬜ pending |
| 04-01-04 | 01 | 1 | MOT-06 | manual | Observe ramp-up/ramp-down on start/stop | N/A | ⬜ pending |
| 04-01-05 | 01 | 1 | MOT-07 | manual | Hold button, release, wait 500ms | N/A | ⬜ pending |
| 04-01-06 | 01 | 1 | NET-01..05 | manual | Boot log + `ping ptzhead.local` + router cycle | N/A | ⬜ pending |
| 04-01-07 | 01 | 1 | SPD-01..03 | manual | Companion preset buttons 0/1/2 | N/A | ⬜ pending |
| 04-01-08 | 01 | 1 | FB-01..04 | manual | Companion feedback listeners | N/A | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

Existing infrastructure covers all phase requirements.

No test infrastructure to create — this phase is entirely manual hardware testing. The firmware already compiles cleanly (verified in Phase 3).

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Motor direction correctness | MOT-01..03 | Physical observation required | Send `PAN 1.0`, verify physical direction matches expectation |
| Smooth accel/decel | MOT-06 | Subjective physical observation | Observe motor ramp-up on start, ramp-down on stop |
| <50ms OSC latency | NET-02 | Subjective feel test (per user decision) | Press Companion button, motor responds immediately |
| WiFi reconnect | NET-05 | Physical router power cycle | Power cycle router, observe serial log reconnection |
| mDNS resolution | NET-03 | Cross-device network test | From another device: `ping ptzhead.local` |
| Feedback display in Companion | FB-01..04 | Companion GUI observation | Move axis, observe variable updates in Companion |
| Heap stability over time | PLAT-05 | 30-minute observation window | Note free heap at boot, 15min, 30min — stable = pass |

---

## Validation Sign-Off

- [ ] All tasks have manual verification instructions
- [ ] Sampling continuity: every success criterion has a test step
- [ ] No automated test infrastructure needed (Wave 0 N/A)
- [ ] No watch-mode flags
- [ ] Feedback latency: N/A (human-observed)
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
