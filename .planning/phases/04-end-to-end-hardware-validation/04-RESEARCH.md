# Phase 4: End-to-End Hardware Validation - Research

**Researched:** 2026-04-06
**Domain:** Hardware validation, Bitfocus Companion integration, ESP32 bench testing
**Confidence:** HIGH

## Summary

Phase 4 is a pure validation and integration phase. No new firmware features are built -- all code exists from Phases 1-3. The work is: flash to real hardware, systematically verify every feature against the 7 success criteria, fix any issues found (constant tweaks, timing adjustments), produce a Companion StreamDeck page for the real-world control surface, and document results in VALIDATION.md.

The primary risk is that hardware behavior differs from what was coded blind (pin polarity, motor direction, timing, thermal issues). All fixes should be `constexpr` constant changes in `ptz_config.h` or minor code adjustments -- not rearchitecting modules. The `.companionconfig` export format is not publicly documented enough for programmatic generation; the plan should produce Companion setup instructions and button definitions that the user configures in the Companion GUI, then exports.

**Primary recommendation:** Structure the plan as a serial test checklist (flash -> boot -> serial motors -> OSC -> feedback -> mDNS -> WiFi resilience), fixing issues inline, then building the Companion page and writing the validation report.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Test order: serial motor commands first (PAN/TILT/ZOOM/STOP), then OSC via Companion. Serial-first isolates motor issues from network issues.
- OSC sender: Companion only -- no intermediate generic OSC tool. Validates the real end-to-end path.
- Plan produces a detailed step-by-step test checklist (flash -> boot -> WiFi -> serial motors -> OSC motion -> feedback -> mDNS -> WiFi drop). User checks off each item.
- Plan also produces a `.companionconfig` export file for import into Companion -- 15-button StreamDeck layout.
- Fix issues in code immediately (constants, config, timing values). Don't just document -- ship working firmware.
- Attempt non-trivial fixes within reason (anything short of rearchitecting a module). If bigger, document and create a follow-up phase.
- Produce a VALIDATION.md report with pass/fail per criterion, fixes applied, and any remaining constraints.
- Layout: motion-focused, D-pad style for pan/tilt (center = stop), zoom row, speed preset buttons, status feedback indicators. Classic PTZ remote layout on a 5x3 grid.
- Live feedback: buttons change color when axis is moving (green while held), show active preset and RSSI as text variables. Tests the full feedback loop.
- Send mode: continuous velocity at 10Hz while button held (matches 500ms heartbeat watchdog design). Stop on release as redundant safety.
- All 7 success criteria from the roadmap must pass. No exceptions -- this is the final phase.
- 50ms latency verified by subjective feel (press button, motor responds immediately). No instrumentation.
- WiFi reconnect tested by router power cycle. Verify ESP32 reconnects and motors auto-stop within 500ms heartbeat timeout.

### Claude's Discretion
- Exact Companion button page layout within the D-pad + zoom + presets + status framework
- Test checklist ordering and granularity
- VALIDATION.md format and level of detail
- Whether to include a heap trend observation step (CNMAT concern from Phase 2)

### Deferred Ideas (OUT OF SCOPE)
None -- discussion stayed within phase scope
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| PLAT-05 | Main loop is simplified for single-source OSC input | Already implemented in Phases 2-3. Validation confirms main loop runs: osc.update() -> osc.updateFeedback() -> serial commands -> heartbeat check -> heap log. |
| NET-01..05 | WiFi connect, power save off, mDNS, UDP port, reconnect | Test checklist covers WiFi boot, mDNS resolve, reconnect after router power cycle. Code already in ptz_wifi.cpp. |
| MOT-01..07 | Pan/tilt/zoom velocity, per-axis stop, all-stop, smooth accel, heartbeat watchdog | Serial motor test validates direction/speed/decel. OSC test validates same via Companion. Heartbeat tested by stopping OSC traffic. |
| SPD-01..03 | 3 speed presets, affect velocity+accel, persist until changed | Companion preset buttons (0/1/2) test switching; observable speed difference validates SPD-02. |
| FB-01..04 | Per-axis moving state, preset ID, RSSI, all as integers | Companion feedback listeners on /ptz/status/* addresses validate receipt and display. |
</phase_requirements>

## Standard Stack

This phase does not introduce new libraries. The firmware stack is fixed from Phases 1-3.

### Firmware (no changes)
| Library | Version | Purpose | Status |
|---------|---------|---------|--------|
| espressif32 | 6.10.0 | ESP32 Arduino framework | Locked in platformio.ini |
| FastAccelStepper | 0.31.x | ISR-driven stepper control | Locked |
| CNMAT/OSC | 3.5.8 | OSC message parse/send | Locked (GitHub tag) |
| WiFiManager | 2.0.17+ | Captive portal provisioning | Locked |
| ESPmDNS | (built-in) | mDNS advertisement | Part of Arduino-ESP32 |

### Tools Required
| Tool | Purpose | Notes |
|------|---------|-------|
| PlatformIO CLI | Flash firmware | `pio run --target upload` |
| Serial monitor | Observe boot logs, send serial commands | `pio device monitor` or built-in |
| Bitfocus Companion | OSC sender/receiver, StreamDeck control surface | generic-osc module, UDP to ESP32 IP:8000 |
| StreamDeck (5x3) | Physical button grid | Standard 15-button layout |
| Router with power switch | WiFi reconnect testing | Physical power cycle |

## Architecture Patterns

### Test Sequence Architecture

The validation follows a dependency-ordered sequence. Each stage must pass before proceeding to the next, because later stages depend on earlier ones working.

```
Stage 1: Flash & Boot
  pio run --target upload
  Serial monitor: "Setup complete" + WiFi connected (or captive portal)

Stage 2: Serial Motor Test (isolates motor from network)
  PAN 1.0 / PAN -1.0 / PAN 0.5 / STOP
  TILT 1.0 / TILT -1.0 / STOP
  ZOOM 1.0 / ZOOM -1.0 / STOP
  Verify: direction, speed scaling, smooth decel, auto-disable ~500ms

Stage 3: OSC Motion via Companion
  Configure generic-osc connection to ESP32 IP:8000
  Test /ptz/pan, /ptz/tilt, /ptz/zoom with float values
  Test /ptz/stop, per-axis stops
  Verify: <50ms latency (subjective feel)

Stage 4: Feedback Verification
  Observe /ptz/status/pan/moving (0/1) in Companion variables
  Observe /ptz/status/preset in Companion variables
  Observe /ptz/status/rssi in Companion variables
  Verify: on-change updates + 1s periodic snapshot

Stage 5: mDNS
  From another device: ping ptzhead.local or dns-sd -B _osc._udp

Stage 6: WiFi Resilience
  Power cycle router while motors are stopped
  Verify: ESP32 reconnects, mDNS re-advertises
  Power cycle router while motors are moving
  Verify: heartbeat watchdog stops motors within 500ms

Stage 7: Build Companion StreamDeck Page
  Configure 15-button layout in Companion GUI
  Export as .companionconfig
```

### Companion StreamDeck 5x3 Layout

Recommended D-pad + zoom + presets + status layout:

```
Row 1 (top):    [Slow]     [Medium]   [Fast]     [RSSI]     [Preset]
Row 2 (mid):    [  <  ]    [  ^  ]    [ STOP ]   [  v  ]    [  >  ]
Row 3 (bot):    [Zoom In]  [Zoom Out] [  --  ]   [  --  ]   [  --  ]
```

Alternative (more natural D-pad):
```
Row 1 (top):    [Slow]     [  ^  ]    [Fast]     [Zoom In]  [RSSI]
Row 2 (mid):    [  <  ]    [ STOP ]   [  >  ]    [Zoom Out] [Preset]
Row 3 (bot):    [Medium]   [  v  ]    [  --  ]   [  --  ]   [  --  ]
```

### Button Configuration Pattern (generic-osc module)

Each motion button needs:
- **Press action:** generic-osc "Send float" to the axis address (e.g., `/ptz/pan` with value `1.0`)
- **Release action:** generic-osc "Send float" to the axis address with value `0.0` (redundant stop on release)
- **No repeat/hold interval needed in Companion** -- the heartbeat watchdog design means a single press command is sufficient. The 500ms timeout auto-stops if no further commands arrive. However, for continuous smooth motion, Companion should repeat at ~100ms (10Hz). Companion's "Repeat interval" on press actions achieves this.

Each preset button needs:
- **Press action:** generic-osc "Send integer" to `/ptz/speed/preset` with value 0, 1, or 2

Each status indicator needs:
- **Feedback:** generic-osc OSC listener on the corresponding `/ptz/status/*` address
- **Button style feedback:** Change background color based on received value (green=1 for moving, default for 0)
- **Text variable:** Display `$(generic-osc:latest_received_args)` or custom variable for RSSI/preset

### Fix-or-Document Decision Tree

```
Issue found during testing?
  |
  +-- Is it a constant/config value? (pin polarity, timing, inversion)
  |     YES -> Fix in ptz_config.h immediately
  |
  +-- Is it a minor code fix? (off-by-one, missing null check, wrong comparison)
  |     YES -> Fix in source, rebuild, re-test
  |
  +-- Is it a design issue requiring module rearchitecture?
  |     YES -> Document in VALIDATION.md as known constraint, create follow-up
  |
  +-- Is it a hardware limitation? (thermal, noise, mechanical)
        YES -> Document in VALIDATION.md as hardware constraint
```

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Companion page config | Custom JSON generator for .companionconfig | Companion GUI + Export | Format is undocumented, version-dependent, and changes between releases |
| OSC testing tool | Custom Python/Node OSC sender | Companion generic-osc module | User decision: Companion-only, validates real end-to-end path |
| Latency measurement | Custom timing instrumentation | Subjective feel test | User decision: press button, motor responds immediately = pass |
| WiFi reconnect test | Automated network drop script | Router power cycle | User decision: physical test, simple and definitive |

**Key insight:** The `.companionconfig` format is an internal Companion export format without public schema documentation. Attempting to generate it programmatically is fragile and likely to break across Companion versions. The plan should provide exact button configuration specs (addresses, values, feedback setup) that the user applies in the Companion GUI, then exports.

## Common Pitfalls

### Pitfall 1: Motor Direction Inversion
**What goes wrong:** Pan direction is inverted relative to expectation. `kInvertPan = true` may need to be `false` (or vice versa) on real hardware.
**Why it happens:** Direction depends on motor wiring and gear orientation -- impossible to know without hardware.
**How to avoid:** Test PAN 1.0 first, observe physical direction, flip `kInvertPan` if needed. Same for tilt/zoom if direction config is added.
**Warning signs:** Motor moves opposite to expected direction on first serial test.

### Pitfall 2: Enable Pin Active-Low Assumption
**What goes wrong:** Motor drivers don't enable or stay enabled permanently.
**Why it happens:** `setEnablePin(pin, true)` in FastAccelStepper sets active-low. If the driver board uses active-high enable, motors won't move.
**How to avoid:** Observe enable pin behavior with serial commands. If motor doesn't move but step/dir pins are toggling, flip the active-low flag.
**Warning signs:** No motor movement despite serial commands, or motor always energized (hot).

### Pitfall 3: Auto-Disable Timing
**What goes wrong:** Motors disable too quickly (lose holding torque during pauses) or too slowly (overheat).
**Why it happens:** `setDelayToDisable(500)` is 500ms -- may need tuning for the specific motor/driver/load.
**How to avoid:** After STOP command, observe serial log for disable timing. Feel motor shaft -- should be free to turn after ~500ms. Adjust constant if needed.
**Warning signs:** Motor shaft stays locked for too long, or PTZ head drifts after stopping.

### Pitfall 4: WiFi Credentials Lost After Framework Switch
**What goes wrong:** ESP32 doesn't auto-connect on first Phase 4 boot -- goes to captive portal.
**Why it happens:** Phase 1 switched from Bluepad32-patched to standard espressif32. NVS layout may differ.
**How to avoid:** Expected behavior on first flash. Re-provision via captive portal. Not a bug -- document as expected.
**Warning signs:** Boot log shows "Starting provisioning portal" instead of "Connected via stored credentials".

### Pitfall 5: CNMAT/OSC Heap Leak
**What goes wrong:** Free heap trends downward over a 30-minute session, eventually crashes.
**Why it happens:** CNMAT/OSC allocates heap per message. `msg.empty()` is called in sendScalarInt, but if any code path misses cleanup, heap leaks.
**How to avoid:** Monitor heap log output (60s interval, already in firmware). Note values at boot, 10min, 30min. Stable = pass. Declining trend = investigate.
**Warning signs:** HEAP log shows progressively lower free values. ESP32 crashes or WiFi becomes unreliable.

### Pitfall 6: mDNS Stops Responding After ~2 Minutes
**What goes wrong:** `ptzhead.local` resolves initially but stops responding.
**Why it happens:** Known ESP32 mDNS bug. Already mitigated by restart-on-GOT_IP pattern in ptz_wifi.cpp.
**How to avoid:** Test mDNS at boot, then again after 5 minutes. If it stops, the GOT_IP restart pattern isn't firing. Check event handler registration timing.
**Warning signs:** `ping ptzhead.local` works immediately after boot but fails minutes later.

### Pitfall 7: Companion Feedback Port Mismatch
**What goes wrong:** Companion sends OSC but doesn't receive feedback.
**Why it happens:** Firmware replies to the ephemeral source port of the last received packet. If Companion's generic-osc module doesn't listen on that port, feedback is lost.
**How to avoid:** generic-osc module in Companion listens on its configured port. The firmware sends to `s_udp.remoteIP():s_udp.remotePort()` -- this should match where Companion is listening. Verify by checking Companion's OSC variables after sending a command.
**Warning signs:** Motor moves on OSC command but Companion shows no feedback variables updating.

### Pitfall 8: Companion Button Repeat Rate
**What goes wrong:** Motor moves briefly then stops (heartbeat timeout).
**Why it happens:** Companion button sends only one OSC message on press, then nothing. 500ms heartbeat watchdog fires.
**How to avoid:** Configure Companion button with press action repeat interval of 100ms (10Hz). This keeps the heartbeat alive while held.
**Warning signs:** Motor starts then stops after ~500ms despite button being held.

## Code Examples

### Serial Test Commands (already implemented in main.cpp)
```
PAN 1.0      # Full speed pan positive
PAN -0.5     # Half speed pan negative
TILT 1.0     # Full speed tilt positive
ZOOM 0.3     # Slow zoom
STOP         # All axes stop with deceleration
WIFI RESET   # Re-enter captive portal
```

### Companion generic-osc Connection Setup
```
Connection type: generic-osc
Target IP: <ESP32 IP from serial log, or ptzhead.local>
Target port: 8000
Protocol: UDP
```

### Companion Button Action: Pan Left (press + release)
```
Press action:
  Type: Send float
  Path: /ptz/pan
  Value: -1.0
  Repeat interval: 100ms (for heartbeat compatibility)

Release action:
  Type: Send float
  Path: /ptz/pan
  Value: 0.0
```

### Companion Button Action: Speed Preset (press only)
```
Press action:
  Type: Send integer
  Path: /ptz/speed/preset
  Value: 0 (slow) / 1 (medium) / 2 (fast)
```

### Companion Feedback: Axis Moving Indicator
```
Feedback type: OSC listener (integer)
Listen path: /ptz/status/pan/moving
When value = 1: Background color = green
When value = 0: Background color = default
```

### Companion Feedback: RSSI Display
```
Button text: RSSI\n$(generic-osc:latest_received_args)
Feedback type: OSC listener (integer)
Listen path: /ptz/status/rssi
```

### Expected Serial Log on Successful Boot
```
[I] BOOT: PTZHead starting
[I] MOTION: 3-axis FastAccelStepper initialized
[I] MOTION: preset=1 maxSps pan=2400 tilt=2400 zoom=2400
[I] WIFI: Trying stored credentials
[I] WIFI: Connecting status=6
[I] WIFI: Connected via stored credentials
[I] WIFI: Connected SSID=MyNetwork IP=192.168.1.xxx RSSI=-55
[I] WIFI: GOT_IP 192.168.1.xxx
[I] MDNS: advertising ptzhead.local _osc._udp:8000
[I] OSC: listening on UDP port 8000
[I] BOOT: Setup complete -- OSC on UDP 8000; serial: PAN/TILT/ZOOM, STOP, WIFI RESET
```

## State of the Art

This section is not applicable -- Phase 4 uses existing firmware without library changes.

## Open Questions

1. **Companion press action repeat interval**
   - What we know: Companion supports repeat intervals on press actions. The firmware needs ~10Hz (100ms) OSC traffic to keep heartbeat alive.
   - What's unclear: The exact UI path in Companion to configure repeat interval on a generic-osc press action. May be called "Relative delay" or "Hold interval" depending on Companion version.
   - Recommendation: User discovers this in Companion GUI during setup. Document as a test checklist item: "verify button repeat rate keeps motor running while held."

2. **Companion .companionconfig programmatic generation**
   - What we know: The format is undocumented internal JSON. Export is done via Companion GUI.
   - What's unclear: Whether the user expects a pre-built file or setup instructions.
   - Recommendation: Plan produces detailed button-by-button configuration specs. User builds in Companion GUI and exports. If a sample .companionconfig from a previous project exists, we could adapt it, but generating from scratch is fragile.

3. **CNMAT heap stability over extended sessions**
   - What we know: `msg.empty()` is called after every TX. Heap is logged every 60s.
   - What's unclear: Whether the 1s feedback snapshot (5 messages/second) causes measurable heap pressure over 30+ minutes.
   - Recommendation: Include a heap trend observation step in the test checklist. Note free heap at boot and after 15-30 minutes of active use. Stable = pass. Declining = flag for investigation.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Manual hardware test checklist (no automated unit tests) |
| Config file | None -- checklist is in PLAN.md |
| Quick run command | `pio run --target upload && pio device monitor` |
| Full suite command | Complete test checklist walkthrough (human-driven) |

### Phase Requirements to Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| PLAT-05 | Main loop simplified for single-source OSC | manual-only | Visual inspection of main.cpp + boot log | N/A |
| MOT-01 | Pan velocity via OSC | manual | Send `/ptz/pan 1.0` from Companion, observe motor | N/A |
| MOT-02 | Tilt velocity via OSC | manual | Send `/ptz/tilt 1.0` from Companion, observe motor | N/A |
| MOT-03 | Zoom velocity via OSC | manual | Send `/ptz/zoom 1.0` from Companion, observe motor | N/A |
| MOT-04 | Per-axis stop | manual | Send `/ptz/pan/stop`, observe axis stops | N/A |
| MOT-05 | All-stop | manual | Send `/ptz/stop`, observe all axes stop | N/A |
| MOT-06 | Smooth accel/decel | manual | Observe ramp-up on start, ramp-down on stop | N/A |
| MOT-07 | Heartbeat watchdog auto-stop | manual | Hold button, release, wait 500ms, observe stop | N/A |
| SPD-01 | 3 speed presets | manual | Send preset 0/1/2, observe speed difference | N/A |
| SPD-02 | Presets affect velocity + accel | manual | Compare slow vs fast preset motor behavior | N/A |
| SPD-03 | Preset persists until changed | manual | Set preset, send motion, verify same speed | N/A |
| NET-01 | WiFi via WiFiManager | manual | Boot, observe captive portal or auto-connect | N/A |
| NET-02 | WiFi power save disabled | manual | Verify <50ms OSC latency (subjective) | N/A |
| NET-03 | mDNS ptzhead.local | manual | `ping ptzhead.local` or `dns-sd -B _osc._udp` from another device | N/A |
| NET-04 | OSC on UDP 8000 | manual | Companion connects to port 8000, commands work | N/A |
| NET-05 | WiFi auto-reconnect | manual | Power cycle router, observe reconnection in serial log | N/A |
| FB-01 | Per-axis moving state feedback | manual | Move axis, observe Companion variable = 1; stop, observe = 0 | N/A |
| FB-02 | Speed preset feedback | manual | Change preset, observe Companion variable updates | N/A |
| FB-03 | RSSI feedback | manual | Observe Companion variable shows negative dBm value | N/A |
| FB-04 | All feedback as integers | manual | Verify Companion receives integer type tags | N/A |

### Sampling Rate
- **Per task commit:** `pio run` (compile check only -- actual validation is human-driven)
- **Per wave merge:** Full test checklist re-run
- **Phase gate:** All 7 success criteria pass before sign-off

### Wave 0 Gaps
None -- this phase is entirely manual hardware testing. No test infrastructure to create. The firmware already compiles cleanly (verified in Phase 3). The "test framework" is the human-driven checklist in the plan.

## Sources

### Primary (HIGH confidence)
- Project source code: `src/main.cpp`, `src/ptz_config.h`, `src/ptz_motion.cpp`, `src/ptz_osc.cpp`, `src/ptz_wifi.cpp` -- full firmware reviewed
- `.planning/HANDOFF.json` -- Phase 3 verification items carried forward
- `.planning/ROADMAP.md` -- Phase 4 success criteria (7 items)
- `.planning/REQUIREMENTS.md` -- Full v1 requirement traceability
- Phase 1/2/3 CONTEXT.md files -- Deferred hardware risks and design decisions

### Secondary (MEDIUM confidence)
- [Bitfocus Companion generic-osc module](https://github.com/bitfocus/companion-module-generic-osc) -- Actions: send float/int/string, feedbacks: OSC listeners, variables: latest_received_args
- [Companion Controls System](https://deepwiki.com/bitfocus/companion/3.1-buttons-and-actions) -- Button types, press/release actions, duration groups
- [Companion Actions Documentation](https://companion.free/user-guide/v4.2/config/buttons/creating/actions/) -- Press actions, release actions, repeat/hold behavior

### Tertiary (LOW confidence)
- `.companionconfig` export format -- No public schema documentation found. Format is internal to Companion and version-dependent.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- No changes, all libraries locked from prior phases
- Architecture: HIGH -- Test sequence is well-defined by success criteria and user decisions
- Pitfalls: HIGH -- Hardware risks explicitly identified and carried from Phases 1-3
- Companion integration: MEDIUM -- generic-osc module capabilities verified, but exact GUI workflow for repeat intervals and feedback setup depends on Companion version

**Research date:** 2026-04-06
**Valid until:** 2026-05-06 (stable -- no library changes, hardware testing is timeless)
