# Project Research Summary

**Project:** PTZHead v2 — OSC Overhaul
**Domain:** ESP32 embedded firmware, OSC-controlled PTZ camera head with Bitfocus Companion integration
**Researched:** 2026-04-03
**Confidence:** HIGH

## Executive Summary

PTZHead v2 is a brownfield firmware overhaul that replaces a Bluetooth gamepad + WebSocket architecture with OSC-over-UDP control from Bitfocus Companion. The project is well-scoped: the existing motion control layer (`ptz_motion` + AccelStepper) is proven and stays. What changes is the input layer (gamepad → OSC), the network protocol (WebSocket/JSON → OSC/UDP), and three modules are deleted outright (gamepad, WebSocket, ownership arbitrator). The recommended approach is a staged, dependency-ordered replacement that keeps the firmware in a compiling, bootable state at every step.

The recommended stack is minimal: standard espressif32 Arduino framework (drop the Bluepad32-patched core), CNMAT/OSC for message parsing, WiFiUDP + ESPmDNS from the bundled Arduino core. The three libraries being removed (ArduinoJson, WebSockets, Bluepad32 framework) are cleanly replaced by the single CNMAT/OSC addition. No framework migration, no RTOS rewrite — just swap the input layer.

The primary risks are reliability-related: WiFi power save causing latency spikes that violate the sub-50ms response requirement, AccelStepper getting starved by network processing overhead, and UDP packet loss creating unsafe motion scenarios. All three have clear mitigations that must be built into the design from the start, not retrofitted. The most contentious technical decision is whether to switch from AccelStepper to FastAccelStepper; research identifies this as the highest-impact reliability improvement but the STACK.md and PITFALLS.md are in tension on it — this is the one gap that needs an early validation spike.

## Key Findings

### Recommended Stack

The standard espressif32 platform (v6.13.0) with Arduino framework replaces the current Bluepad32-patched setup. The only new dependency is CNMAT/OSC v3.5.8, the de facto Arduino OSC library with ESP32 examples and full spec support. WiFiUDP and ESPmDNS ship bundled with the Arduino core — no additional dependencies needed. WiFiManager (already working) and AccelStepper (already proven) stay untouched.

The critical platform change is removing the `platform_packages` block that overrides the Arduino core with the Bluepad32 fork. This enables the standard toolchain and eliminates the gamepad dependency entirely. Three libraries exit the project: ArduinoJson, WebSockets, and the Bluepad32 framework override.

**Core technologies:**
- `espressif32@6.13.0` + Arduino framework: Platform — drop-in upgrade from current 6.10.0, removes Bluepad32 dependency
- `CNMAT/OSC@3.5.8`: OSC parsing — battle-tested, full spec, ESP32-compatible, preferred over MicroOsc (less mature) and ArduinoOSC (over-engineered)
- `WiFiUDP` (bundled): UDP transport — standard, no fragmentation risk for OSC packet sizes (<100 bytes)
- `ESPmDNS` (bundled): mDNS advertisement — zero-config discovery, requires `MDNS.addService()` to prevent 2-minute TTL expiry
- `WiFiManager@^2.0.17`: WiFi provisioning — already in use, keep as-is
- `AccelStepper@^1.64`: Stepper control — already proven at 4000 sps; see AccelStepper starvation gap below

### Expected Features

The MVP replaces the entire input layer while preserving all motion behavior. Companion integration is via the generic-osc module — no custom Companion module needed for v1.

**Must have (table stakes):**
- Per-axis velocity control (`/ptz/pan f`, `/ptz/tilt f`, `/ptz/zoom f`) — core interaction model
- All-stop and per-axis stop — safety requirement
- Speed presets switchable via OSC (`/ptz/speed/preset i`) — operator workflow
- OSC moving-state feedback (per-axis integer booleans) — Companion button LED feedback
- mDNS advertisement (`ptzhead.local`, `_osc._udp` service) — zero-config discovery
- Connection loss watchdog with auto-stop — safety net (must be in core, not deferred)
- Remove gamepad, WebSocket, owner modules — prerequisite for clean architecture

**Should have (differentiators for v1.x):**
- Diagonal movement convenience commands — Companion PTZ modules all offer 8-way movement
- WiFi RSSI feedback — in-show network diagnostics
- Configurable OSC ports via captive portal parameters — eliminates recompile for port changes
- Axis inversion via OSC with NVS persistence — variable mounting configurations
- Full status broadcast bundle (periodic OSCBundle with all state)

**Defer (v2+):**
- Position presets and absolute positioning — requires encoders (hardware change)
- Multi-device sync — no use case until multiple heads exist
- Custom Companion module — defer until OSC namespace is stable and proven

**Anti-features to explicitly avoid:**
- Web UI dashboard (Companion is the dashboard)
- TCP OSC transport (defeats real-time purpose)
- OTA firmware updates (flash complexity, security burden for single device)
- Multi-client arbitration (removed for good reason; single-source model is correct)

### Architecture Approach

The architecture retains the existing module pattern — loosely coupled `ptz::` namespace classes orchestrated by a thin `main.cpp` loop. The overhaul deletes three modules (`ptz_gamepad`, `ptz_ws`, `ptz_owner`) and adds two (`ptz_osc`, `ptz_mdns`). The new `ptz_osc` module replaces both the WebSocket input and the ownership arbitrator, handling all UDP receive, OSC parsing, dispatch, and status feedback. `ptz_motion` requires only minor additions: per-axis velocity setters and a `applySpeedPreset()` method.

**Major components:**
1. `ptz_osc` (new): All OSC I/O — parses inbound commands, dispatches to motion, sends status bundles to reply-to-sender IP
2. `ptz_motion` (refactored): 3-axis stepper velocity control, speed preset application, moving state exposure
3. `ptz_mdns` (new, tiny): `MDNS.begin()` + `MDNS.addService()` wrapper — critical that `addService()` is called to prevent TTL expiry
4. `ptz_wifi` (kept): WiFiManager provisioning plus event-based reconnection logic (to be added)
5. `ptz_config` (extended): Speed preset structs, OSC port constant, address string constants
6. `main.cpp` (simplified): Loses ~60 lines of ownership/gamepad orchestration; gains cleaner `osc.update()` → `motion.update()` → `motion.run()` loop

The feedback target is discovered dynamically: when any OSC message arrives, the sender's IP and port are cached and used for all subsequent status broadcasts. No configuration of Companion's IP is needed on the device side.

### Critical Pitfalls

1. **WiFi power save causes 100-300ms UDP latency** — Call `esp_wifi_set_ps(WIFI_PS_NONE)` and `WiFi.setSleep(false)` immediately after WiFi connects. Non-negotiable for sub-50ms response. Also fixes mDNS TTL expiry (Pitfall 4 shares this fix).

2. **AccelStepper starved by network processing overhead** — AccelStepper is software-timed; any loop iteration >250µs at 4000 sps causes missed steps. FastAccelStepper (hardware RMT/MCPWM) eliminates this completely. STACK.md recommends staying with AccelStepper; PITFALLS.md recommends FastAccelStepper. This must be resolved in Phase 1 with a timing benchmark spike.

3. **UDP packet loss with no safety recovery** — Implement a heartbeat watchdog: if no OSC message received for 500-750ms, stop all axes. Have Companion send velocity continuously (10-20Hz) while button held, not just on press. Send redundant stop commands on button release. This must be in the core OSC phase, not deferred.

4. **mDNS stops responding after ~2 minutes** — Always call `MDNS.addService()` after `MDNS.begin()`. Disabling WiFi power save (Pitfall 1 fix) also resolves this. Always configure a fixed IP fallback in Companion — mDNS is convenience, not reliability.

5. **Framework switch breaks NVS/partition layout** — Document current partition table before switching. Test WiFiManager credential persistence after the switch. Keep the existing `WIFI RESET` serial command as the recovery path.

## Implications for Roadmap

Based on research, the build order from ARCHITECTURE.md maps directly to phases. Each phase produces testable firmware.

### Phase 1: Platform Switch and Cleanup
**Rationale:** Prerequisite for everything. Removes Bluepad32 dependency and reduces the codebase to a stable, compiling baseline without any input source. Also the right moment to validate AccelStepper vs FastAccelStepper — benchmark loop timing with WiFi active before committing to either path.
**Delivers:** Firmware that boots, connects to WiFi, runs motors with no input source, and compiles cleanly against standard espressif32.
**Addresses:** Remove legacy modules (P1); framework migration prerequisite
**Avoids:** NVS/partition incompatibility (Pitfall 5) — compare partition tables before and after; AccelStepper starvation decision (Pitfall 2) — resolve via timing benchmark spike

### Phase 2: WiFi Reliability Hardening
**Rationale:** OSC reliability cannot be evaluated until WiFi is solid. Power save must be disabled before any latency testing. Reconnection logic prevents the device from requiring a physical power cycle after any network hiccup.
**Delivers:** WiFi that disables power save on connect, reconnects automatically via event handler with exponential backoff, and stops motors immediately on disconnect.
**Addresses:** Emergency auto-stop on connection loss (P1); connection/alive status (P1)
**Avoids:** WiFi power save latency (Pitfall 1), no auto-reconnect (Pitfall 7), mDNS TTL expiry (Pitfall 4 — same fix)

### Phase 3: OSC Input (Commands)
**Rationale:** Core value delivery. With WiFi solid and the platform clean, build the OSC receive path and motor dispatch. Per-axis velocity, combined move, stop commands, and speed presets are all implemented here.
**Delivers:** Firmware controllable via any OSC sender (oscsend, Protokol, etc.) — motors respond to `/ptz/pan`, `/ptz/tilt`, `/ptz/zoom`, `/ptz/move`, `/ptz/stop`, `/ptz/speed/preset`.
**Uses:** CNMAT/OSC, WiFiUDP, reply-to-sender IP caching pattern
**Implements:** `ptz_osc` receive path; per-axis velocity setters and `applySpeedPreset()` in `ptz_motion`
**Avoids:** Heap allocation in OSC hot path (Pitfall 6) — pre-allocate static receive buffer, monitor free heap; UDP buffer overflow (Pitfall 8) — drain all packets per loop iteration; bad namespace design (Pitfall 9) — use the designed `/ptz/` schema

### Phase 4: OSC Output (Status Feedback)
**Rationale:** Companion button LEDs require feedback. The heartbeat watchdog (safety) also lives here — it cannot function without OSC traffic flowing bidirectionally.
**Delivers:** Status broadcast at 20Hz via OSCBundle to reply-to-sender IP; heartbeat watchdog that stops all axes on timeout; per-axis moving booleans for Companion feedbacks.
**Implements:** `ptz_osc` send path; `sendStatus()` with OSCBundle; watchdog timer pattern
**Avoids:** UDP packet loss safety risk (Pitfall 3) — watchdog is the recovery mechanism

### Phase 5: mDNS Discovery
**Rationale:** Small, independent, high operator value. Makes the device zero-config discoverable. Placed after core OSC is validated so mDNS can be tested in the context of a fully functional device.
**Delivers:** `ptzhead.local` resolves on the local network; `_osc._udp` service advertised for zero-config Companion setup.
**Implements:** `ptz_mdns` module (tiny wrapper)
**Avoids:** mDNS TTL expiry (Pitfall 4) — `MDNS.addService()` is mandatory, not optional

### Phase 6: Companion Integration and End-to-End Validation
**Rationale:** Not firmware development — Companion configuration and end-to-end validation. Proves the system works under real StreamDeck button-hold conditions, validates the heartbeat watchdog fires correctly, confirms button LED feedback works.
**Delivers:** Companion page with directional buttons, speed preset selectors, and moving-state feedbacks wired to OSC. Full end-to-end: StreamDeck button hold → motor movement → button LED lights up → release → motor stops within 50ms.
**Addresses:** All P1 features validated together

### Phase Ordering Rationale

- Phase 1 must be first: the Bluepad32 platform dependency blocks all other work and the AccelStepper vs FastAccelStepper decision must be resolved before implementing the OSC receive loop.
- Phase 2 before Phase 3: WiFi latency corrupts all OSC timing tests; disabling power save is a one-time configuration that must be in place before any OSC latency measurement is meaningful.
- Phase 3 before Phase 4: Cannot send feedback without first receiving commands (reply-to-sender pattern requires at least one inbound packet).
- Phase 5 is independent but placed late because it depends on nothing and adds operational convenience without affecting core functionality.
- Phase 6 is integration testing, not firmware — placed last by definition.

### Research Flags

Phases likely needing deeper research during planning:
- **Phase 1 (Platform Switch):** The AccelStepper vs FastAccelStepper decision needs a timing spike — benchmark loop iteration time with WiFi active and OSC parsing in the loop before committing. FastAccelStepper has a different API and would require `ptz_motion` refactoring.
- **Phase 3 (OSC Input):** CNMAT/OSC heap allocation behavior needs validation — log `ESP.getFreeHeap()` over a 30-minute session and confirm it is stable before declaring the implementation done.

Phases with well-documented patterns (skip research-phase):
- **Phase 2 (WiFi Hardening):** WiFi event handler pattern and `WIFI_PS_NONE` are well-documented ESP32 behaviors with official examples.
- **Phase 4 (OSC Output):** OSCBundle pattern and reply-to-sender IP caching are standard; ARCHITECTURE.md provides complete implementation sketches.
- **Phase 5 (mDNS):** `MDNS.begin()` + `MDNS.addService()` is a 5-line implementation.
- **Phase 6 (Companion Integration):** Companion generic-osc module is well-documented; this is configuration work.

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | All library versions verified against GitHub releases. espressif32 6.13.0 released Feb 2025. CNMAT/OSC 3.5.8 verified. The AccelStepper vs FastAccelStepper call is the only unresolved technical question. |
| Features | HIGH | Derived from existing codebase analysis plus Companion module documentation. OSC namespace designed from Panasonic PTZ module patterns. Clear MVP vs v1.x vs v2+ split. |
| Architecture | HIGH | Build order validated by dependency chain. Module boundaries follow existing codebase patterns. PtzOsc API sketch provided. Main loop simplification is concrete (~60 lines removed). |
| Pitfalls | HIGH | All pitfalls backed by specific GitHub issues on arduino-esp32 and esp-idf repos. Not theoretical — documented production failures with reproducible detection steps. |

**Overall confidence:** HIGH

### Gaps to Address

- **AccelStepper vs FastAccelStepper:** STACK.md recommends keeping AccelStepper; PITFALLS.md recommends switching to FastAccelStepper. These are not reconciled. Resolution: implement a timing benchmark in Phase 1 — measure worst-case loop iteration time with WiFi active and OSC parsing running. If any iteration exceeds 200µs, switch to FastAccelStepper before Phase 3.
- **CNMAT/OSC heap behavior:** PITFALLS.md flags heap fragmentation risk from CNMAT/OSC's internal `OSCData` allocation. STACK.md recommends CNMAT/OSC over MicroOsc. Resolution: validate with `ESP.getFreeHeap()` monitoring during Phase 3. If heap trends downward over 30 minutes, evaluate switching to MicroOsc at that point (it uses in-place parsing with no internal allocation).
- **WiFi credential persistence across framework switch:** Bluepad32 core to standard espressif32 may change NVS layout. Must verify before committing the switch with a test device.

## Sources

### Primary (HIGH confidence)
- [CNMAT/OSC GitHub](https://github.com/CNMAT/OSC) — v3.5.8 release, ESP32 compatibility
- [espressif32 platform releases](https://github.com/platformio/platform-espressif32/releases) — v6.13.0 release date and contents
- [arduino-esp32 ESPmDNS](https://github.com/espressif/arduino-esp32/blob/master/libraries/ESPmDNS/src/ESPmDNS.h) — bundled with Arduino core
- [Companion generic-osc module HELP.md](https://github.com/bitfocus/companion-module-generic-osc/blob/master/companion/HELP.md) — feedback and variable capabilities
- [Companion Panasonic PTZ module](https://github.com/bitfocus/companion-module-panasonic-ptz) — reference PTZ feature set
- Existing codebase: `src/main.cpp`, `src/ptz_motion.cpp`, `src/ptz_ws.h`, `src/ptz_config.h`

### Secondary (MEDIUM confidence)
- [ESP32 mDNS stops after 2 min — arduino-esp32#4406](https://github.com/espressif/arduino-esp32/issues/4406) — well-documented bug, workaround confirmed
- [mDNS unreliable without WiFi.setSleep(false) — arduino-esp32#7156](https://github.com/espressif/arduino-esp32/issues/7156) — community-confirmed fix
- [WiFi Auto Reconnect broken — arduino-esp32#653](https://github.com/espressif/arduino-esp32/issues/653) — documented issue, event-handler workaround accepted
- [ESP32 UDP packet bunching — Arduino Forum](https://forum.arduino.cc/t/esp32-wifi-udp-bunching-packets/1162055) — reproduced by multiple users
- [AccelStepper + WiFi starvation — Arduino Forum](https://forum.arduino.cc/t/accelstepper-wifi-problem/978221) — community reproductions

### Tertiary (LOW confidence)
- [FastAccelStepper](https://github.com/gin66/FastAccelStepper) — recommended in PITFALLS.md; not benchmarked against this specific hardware configuration
- [MicroOsc](https://github.com/thomasfredericks/MicroOsc) — zero-allocation alternative; CNMAT/OSC heap behavior in this use case not directly measured

---
*Research completed: 2026-04-03*
*Ready for roadmap: yes*
