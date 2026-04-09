---
phase: 3
slug: feedback-and-discovery
status: planned
nyquist_compliant: true
wave_0_complete: false
created: 2026-04-05
---

# Phase 3 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | PlatformIO compile-only validation (no unit test framework in this Arduino/PlatformIO project) |
| **Config file** | platformio.ini |
| **Quick run command** | `pio run` |
| **Full suite command** | `pio run && pio run -t checkprogsize` |
| **Estimated runtime** | ~15 seconds (incremental) / ~45s (clean build) |

**Rationale:** PTZHead is an Arduino/PlatformIO firmware project with no native unit-test framework. Per-task validation is compile + link + static grep verification of code shape. Behavioral validation (OSC feedback roundtrip, mDNS resolution) requires a live ESP32 and is covered in Phase 4 hardware validation + the optional Wave 0 Python loopback script.

---

## Sampling Rate

- **After every task commit:** Run `pio run` (compile clean, exit 0)
- **After every plan wave:** Run `pio run` + diff inspection (`git diff src/ptz_osc.cpp | grep lastRxMs_` must show no new writes)
- **Before `/gsd:verify-work`:** Full suite green + all grep acceptance criteria satisfied
- **Max feedback latency:** ~15 seconds per task

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 03-01-01 | 01 | 1 | FB-01 | static+compile | `pio run` + grep `isPanMoving` in src/ptz_motion.h | ✅ | ⬜ pending |
| 03-01-02 | 01 | 1 | FB-02, FB-03, FB-04, NET-03 | static+compile | `pio run` + grep constants in src/ptz_config.h | ✅ | ⬜ pending |
| 03-02-01 | 02 | 2 | FB-01, FB-02, FB-03, FB-04 | static+compile | `pio run` + grep `updateFeedback`, `StatusSnapshot`, `lastSenderIp_` in src/ptz_osc.h | ✅ | ⬜ pending |
| 03-02-02 | 02 | 2 | FB-01, FB-02, FB-03, FB-04 | static+compile | `pio run` + grep `remoteIP`, `sendScalarInt`, `msg.empty()`, `lastRxMs_ =` count=2 in src/ptz_osc.cpp | ✅ | ⬜ pending |
| 03-02-03 | 02 | 2 | FB-01, FB-02, FB-03, FB-04 | static+compile | `pio run` + grep `g_osc.updateFeedback()` in src/main.cpp | ✅ | ⬜ pending |
| 03-03-01 | 03 | 2 | NET-03 | static+compile | `pio run` + grep `MDNS.begin`, `MDNS.addService`, `MDNS.setInstanceName` in src/ptz_wifi.cpp | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] (Optional) `scripts/feedback_loopback_test.py` — Python OSC client using `python-osc` that connects to `ptzhead.local:8000`, sends motion commands, listens on ephemeral port, asserts feedback addresses + int type tags + values. NOT blocking for execution; used in Phase 4 hardware validation.
- [ ] (Optional) `scripts/README.md` — document how to run the loopback test against a live ESP32.

*This project intentionally has no unit-test framework — compile validation + hardware loopback is the validation model. Wave 0 is OPTIONAL for Phase 3 execution; required for Phase 4 sign-off.*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| `ptzhead.local` resolves over LAN | NET-03 | Requires live ESP32 on same L2 broadcast domain | `ping ptzhead.local` or `dns-sd -B _osc._udp local.` on macOS / `avahi-browse -r _osc._udp` on Linux after flash |
| mDNS survives >5 minutes uptime | NET-03 | Pitfall 1 regression check; needs wall-clock elapsed time | `while true; do dig @224.0.0.251 -p 5353 ptzhead.local; sleep 30; done` over 10min |
| `/ptz/status/pan/moving` flips on motion | FB-01 | Requires live OSC TX + RX loopback | Python loopback script: send `/ptz/pan 0.5`, listen on ephemeral port, expect `/ptz/status/pan/moving 1` within 100ms |
| `/ptz/status/preset` emits on change | FB-02 | Requires live OSC loopback | Send `/ptz/speed/preset 2`, expect `/ptz/status/preset 2` reply |
| `/ptz/status/rssi` emitted every ~1s | FB-03 | Requires timed packet capture | Listen 3s on ephemeral port after sending any command, expect ≥2 RSSI messages with negative int value |
| All feedback uses OSC int32 type tag 'i' | FB-04 | Wire-level type tag inspection | Packet capture asserts type tag byte after address+padding is 'i' (0x69) |
| Heartbeat invariant (lastRxMs_ RX-only) | FB-01..04 | Architectural check, not runtime | `grep -c "lastRxMs_ =" src/ptz_osc.cpp` must return exactly 2 after Phase 3 execution |

All behavioral verifications deferred to Phase 4 hardware validation.

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify (pio run) — compile-gated per task
- [x] Sampling continuity: every task has automated pio run — no gaps
- [x] Wave 0 optional for Phase 3 (no framework needed; hardware validation is the bar)
- [x] No watch-mode flags
- [x] Feedback latency < 45s (clean build)
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** approved 2026-04-05 (compile-only validation model accepted; behavioral validation deferred to Phase 4)
