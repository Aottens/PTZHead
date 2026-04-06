# Phase 4: End-to-End Hardware Validation - Context

**Gathered:** 2026-04-06
**Status:** Ready for planning

<domain>
## Phase Boundary

Flash firmware to real ESP32 + stepper hardware, bench-test every feature from Phases 1-3, fix issues found, and sign off all 7 success criteria. Produces a Companion button page (.companionconfig) for a standard 15-button StreamDeck and a VALIDATION.md report. No new features — this phase validates and tunes what's already built.

</domain>

<decisions>
## Implementation Decisions

### Test sequence & tooling
- Test order: serial motor commands first (PAN/TILT/ZOOM/STOP), then OSC via Companion. Serial-first isolates motor issues from network issues.
- OSC sender: Companion only — no intermediate generic OSC tool. Validates the real end-to-end path.
- Plan produces a detailed step-by-step test checklist (flash -> boot -> WiFi -> serial motors -> OSC motion -> feedback -> mDNS -> WiFi drop). User checks off each item.
- Plan also produces a `.companionconfig` export file for import into Companion — 15-button StreamDeck layout.

### Fix-or-document policy
- Fix issues in code immediately (constants, config, timing values). Don't just document — ship working firmware.
- Attempt non-trivial fixes within reason (anything short of rearchitecting a module). If bigger, document and create a follow-up phase.
- Produce a VALIDATION.md report with pass/fail per criterion, fixes applied, and any remaining constraints.

### Companion StreamDeck page
- Layout: motion-focused, D-pad style for pan/tilt (center = stop), zoom row, speed preset buttons, status feedback indicators. Classic PTZ remote layout on a 5x3 grid.
- Live feedback: buttons change color when axis is moving (green while held), show active preset and RSSI as text variables. Tests the full feedback loop.
- Send mode: continuous velocity at 10Hz while button held (matches 500ms heartbeat watchdog design). Stop on release as redundant safety.

### Sign-off criteria
- All 7 success criteria from the roadmap must pass. No exceptions — this is the final phase.
- 50ms latency verified by subjective feel (press button, motor responds immediately). No instrumentation.
- WiFi reconnect tested by router power cycle. Verify ESP32 reconnects and motors auto-stop within 500ms heartbeat timeout.

### Claude's Discretion
- Exact Companion button page layout within the D-pad + zoom + presets + status framework
- Test checklist ordering and granularity
- VALIDATION.md format and level of detail
- Whether to include a heap trend observation step (CNMAT concern from Phase 2)

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Success criteria
- `.planning/ROADMAP.md` Phase 4 section — 7 success criteria that all must pass

### Requirements coverage
- `.planning/REQUIREMENTS.md` — Full v1 requirement set; PLAT-05 (main loop cleanup) still pending; all others complete

### Deferred hardware risks (from prior phases)
- `.planning/phases/01-platform-migration-and-cleanup/01-CONTEXT.md` — FastAccelStepper pin config, auto-enable active-low, kInvertPan compile-time constant
- `.planning/phases/02-network-and-core-osc-control/02-CONTEXT.md` — OSC namespace (`/ptz/*`), speed preset values (25/60/100%), heartbeat 500ms, WiFi.setSleep(false), CNMAT heap concern
- `.planning/phases/03-feedback-and-discovery/03-CONTEXT.md` — Feedback addresses (`/ptz/status/*`), int32 scalars, reply-to-sender, mDNS restart-on-GOT_IP, sender captured before buffer drain

### Firmware source
- `src/main.cpp` — Main loop with OSC update, feedback, heartbeat, serial commands
- `src/ptz_config.h` — All pin assignments, motion parameters, OSC/feedback/mDNS constants
- `src/ptz_motion.h` / `src/ptz_motion.cpp` — FastAccelStepper 3-axis motion with per-axis isMoving()
- `src/ptz_osc.h` / `src/ptz_osc.cpp` — CNMAT/OSC dispatch, feedback TX, sender cache
- `src/ptz_wifi.h` / `src/ptz_wifi.cpp` — WiFiManager provisioning, event-driven reconnect, mDNS lifecycle

### Phase 3 handoff (human verification items)
- `.planning/HANDOFF.json` — 3 non-blocking verification items: mDNS resolve on first boot, feedback latency, RSSI emission cadence

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- Serial commands (PAN/TILT/ZOOM/STOP/WIFI RESET) already in main.cpp — first validation stage uses these directly
- `ESP.getFreeHeap()` logging already in main loop at 60s intervals — validates CNMAT heap concern passively
- `ptz::logShouldEmit` rate-limited logging throughout — serial monitor output is already structured for validation

### Established Patterns
- `constexpr` config in `ptz_config.h` — hardware fixes (pin polarity, timing) are constant changes, not code rewrites
- FastAccelStepper auto-enable manages driver enable pins — auto-disable timing is observable via serial log

### Integration Points
- `platformio.ini` upload target — `pio run --target upload` is the flash command
- Companion generic-osc module — connects to ESP32 IP:8000, sends/receives OSC per address schema
- mDNS — `ptzhead.local` should resolve from any device on the same network

</code_context>

<specifics>
## Specific Ideas

- StreamDeck layout should feel like a classic PTZ remote: directional pad in the center, zoom on the side, presets at top or bottom
- Companion buttons use momentary (latching = false) with 10Hz repeat for velocity commands — this is critical for heartbeat watchdog compatibility
- The .companionconfig file should be importable directly into Companion with zero manual setup beyond connecting to the PTZ head's IP/mDNS address
- VALIDATION.md serves as a reference for future hardware revisions — document what worked AND what needed fixing

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 04-end-to-end-hardware-validation*
*Context gathered: 2026-04-06*
