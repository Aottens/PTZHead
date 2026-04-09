# PTZ Head Hardware Validation Test Checklist

**Date:** 2026-04-06
**Firmware:** PTZHead v2 (Phases 1-3 complete)
**Phase 4 Plan:** 04-01 (Test Preparation)

## Prerequisites

### Hardware Required
- ESP32 dev board (esp32dev)
- 3x stepper motor drivers (active-low enable)
- 3x stepper motors (pan, tilt, zoom)
- Elgato StreamDeck (5x3, 15 buttons)
- WiFi router with physical power switch (for resilience testing)
- Computer with:
  - PlatformIO CLI installed
  - Bitfocus Companion installed
  - USB cable for ESP32 serial

### Before Starting
1. Verify `pio run` compiles cleanly (Task 3 of this plan)
2. Have COMPANION-SETUP.md open for reference during Stage 3+
3. Have serial monitor ready: `pio device monitor` (115200 baud)

---

## Stage 1: Flash and Boot (SC-1)

**Success Criterion SC-1:** Firmware flashes cleanly via `pio run --target upload` and boots to `Setup complete` with WiFi connected (or captive portal on first boot)

### Steps

1. Connect ESP32 via USB
2. Flash firmware:
   ```
   pio run --target upload
   ```
3. Open serial monitor:
   ```
   pio device monitor
   ```
4. Observe boot log. Expected output:
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

### First Boot Note
On first boot after Phase 1 framework switch (espressif32 replacing Bluepad32), WiFi credentials may be lost from NVS. The ESP32 will start a captive portal instead of auto-connecting. This is expected (Pitfall 4), not a bug. Connect to the "PTZHead Setup" AP and provision WiFi credentials via the portal. After provisioning, the ESP32 will reboot and connect normally.

### Pass Criteria
- [ ] Firmware uploads without error
- [ ] Serial monitor shows "Setup complete"
- [ ] WiFi connected (or captive portal completed successfully)

**Notes:**
_____________________________________________________________

---

## Stage 2: Serial Motor Test (SC-2)

**Success Criterion SC-2:** Serial motor test commands drive all 3 axes correctly -- direction matches expectation, speed scales linearly, STOP decelerates smoothly, motors auto-disable ~500ms after stop

### Pan Axis Tests

Enter each command in the serial monitor:

| # | Command | Expected Behavior |
|---|---------|-------------------|
| 1 | `PAN 1.0` | Pan motor runs in positive direction at full speed |
| 2 | `PAN -1.0` | Pan motor runs in opposite direction at full speed |
| 3 | `PAN 0.5` | Pan motor runs at roughly half speed |
| 4 | `STOP` | Pan decelerates smoothly to stop (not instant) |
| 5 | Wait ~500ms after stop | Motor shaft should be free to turn by hand (auto-disable) |

**Direction wrong?** Flip `kInvertPan` in `src/ptz_config.h` (currently `true`, change to `false`). Rebuild and re-flash.

**Motor doesn't move at all?** Check enable pin polarity. Current config uses `setEnablePin(pin, true)` (active-low). If your driver uses active-high enable, change the second argument to `false` in `src/ptz_motion.cpp`. See Pitfall 2.

**Auto-disable too fast/slow?** The `setDelayToDisable(500)` value in `src/ptz_motion.cpp` controls how long after stopping the driver stays energized. Increase if the PTZ head drifts after stopping (needs holding torque). Decrease if motors overheat.

### Tilt Axis Tests

| # | Command | Expected Behavior |
|---|---------|-------------------|
| 6 | `TILT 1.0` | Tilt motor runs in positive direction |
| 7 | `TILT -1.0` | Tilt motor runs in opposite direction |
| 8 | `STOP` | Tilt decelerates smoothly |

### Zoom Axis Tests

| # | Command | Expected Behavior |
|---|---------|-------------------|
| 9 | `ZOOM 1.0` | Zoom motor runs in positive direction |
| 10 | `ZOOM -1.0` | Zoom motor runs in opposite direction |
| 11 | `STOP` | Zoom decelerates smoothly |

### Pass Criteria
- [ ] All 3 axes move
- [ ] Direction correct (or noted for fix with `kInvertPan` / direction config)
- [ ] Speed scales with norm value (0.5 is noticeably slower than 1.0)
- [ ] Smooth deceleration on STOP (not instant)
- [ ] Auto-disable ~500ms after stop (motor shaft free to turn, `setDelayToDisable` value confirmed)

**Notes:**
_____________________________________________________________

---

## Stage 3: OSC Motion via Companion (SC-3)

**Success Criterion SC-3:** OSC commands from Companion produce motor motion within the 50ms latency target on WiFi

### Setup

1. Open Companion web UI
2. Add connection: generic-osc
   - Target IP: `<ESP32 IP from serial log>` (or `ptzhead.local` if mDNS confirmed in Stage 6)
   - Target port: `8000`
   - Protocol: UDP
3. Configure buttons per COMPANION-SETUP.md

### Tests

| # | Action | Expected Behavior |
|---|--------|-------------------|
| 1 | Press Pan Right button (sends `/ptz/pan` value `1.0`) | Pan motor responds |
| 2 | Press Tilt Up button (sends `/ptz/tilt` value `1.0`) | Tilt motor responds |
| 3 | Press Zoom In button (sends `/ptz/zoom` value `0.5`) | Zoom motor responds |
| 4 | Press STOP ALL button (sends `/ptz/stop`) | All axes stop |
| 5 | Start tilt moving, then press Pan Stop only (`/ptz/pan/stop`) | Only pan stops, tilt continues |
| 6 | Latency check: press button, observe motor | Motor responds immediately (<50ms subjective feel) |

### Pass Criteria
- [ ] OSC commands produce motor motion
- [ ] Per-axis stop works (one axis stops while another continues)
- [ ] All-stop works
- [ ] Latency feels instant (<50ms subjective)

**Notes:**
_____________________________________________________________

---

## Stage 4: Speed Presets (SC-3 continued)

**Success Criterion SC-3 (continued):** Sending `/ptz/speed/preset` switches the active speed/acceleration profile and the change is immediately observable

### Tests

| # | Action | Expected Behavior |
|---|--------|-------------------|
| 1 | Send `/ptz/speed/preset` value `0` (Slow), then `/ptz/pan` value `1.0` | Motor moves slowly (25% speed) |
| 2 | Send `/ptz/speed/preset` value `2` (Fast), then `/ptz/pan` value `1.0` | Motor moves noticeably faster (100% speed) |
| 3 | Send `/ptz/speed/preset` value `1` (Medium) | Back to default (60% speed) |
| 4 | Set preset, send multiple motion commands | Preset persists until explicitly changed |

### Pass Criteria
- [ ] 3 distinct speed levels observable (slow/medium/fast)
- [ ] Preset persists across commands

**Notes:**
_____________________________________________________________

---

## Stage 5: Feedback Verification (SC-4)

**Success Criterion SC-4:** Status feedback (axis moving flags, speed preset, RSSI) arrives at the OSC sender and updates in real time

### Prerequisites
- Companion generic-osc connection active from Stage 3
- Companion variables/feedbacks configured per COMPANION-SETUP.md

### Tests

| # | Action | Expected Behavior |
|---|--------|-------------------|
| 1 | Move pan axis via Companion button | `/ptz/status/pan/moving` shows `1` in Companion |
| 2 | Stop pan axis | `/ptz/status/pan/moving` shows `0` |
| 3 | Move tilt axis | `/ptz/status/tilt/moving` shows `1` |
| 4 | Stop tilt axis | `/ptz/status/tilt/moving` shows `0` |
| 5 | Move zoom axis | `/ptz/status/zoom/moving` shows `1` |
| 6 | Stop zoom axis | `/ptz/status/zoom/moving` shows `0` |
| 7 | Change speed preset to 0 | `/ptz/status/preset` shows `0` |
| 8 | Change speed preset to 2 | `/ptz/status/preset` shows `2` |
| 9 | Observe RSSI display | `/ptz/status/rssi` shows negative dBm integer (e.g., `-55`) |
| 10 | Wait 5+ seconds with no motion | Periodic snapshot still emits (~1s interval). RSSI keeps updating. |

### Handoff Verification Items (from Phase 3)
- [ ] Feedback latency is within one loop tick (effectively instant when axis starts/stops)
- [ ] RSSI emits at ~1s cadence (observable in Companion variable updates)

### Pass Criteria
- [ ] Per-axis moving state updates correctly (1 when moving, 0 when stopped)
- [ ] Preset feedback works (shows current preset index)
- [ ] RSSI shows valid negative dBm value
- [ ] All values are integers (no floats) -- FB-04 compliance

**Notes:**
_____________________________________________________________

---

## Stage 6: mDNS Discovery (SC-6)

**Success Criterion SC-6:** mDNS resolves `ptzhead.local` from another device on the network

### Tests

From a separate device on the same WiFi network:

| # | Command / Action | Expected Result |
|---|------------------|-----------------|
| 1 | macOS: `dns-sd -B _osc._udp` | Shows PTZHead instance |
| 2 | macOS: `ping ptzhead.local` | Resolves to ESP32 IP address |
| 3 | Linux: `avahi-browse -r _osc._udp` | Shows PTZHead service |
| 4 | Linux: `ping ptzhead.local` | Resolves to ESP32 IP address |
| 5 | Wait 5 minutes, then ping again | Still resolves (verifies GOT_IP restart pattern prevents 2-minute mDNS silence bug -- Pitfall 6) |

### Pass Criteria
- [ ] `ptzhead.local` resolves to ESP32 IP
- [ ] `_osc._udp` service discoverable
- [ ] Still resolves after 5 minutes

**Notes:**
_____________________________________________________________

---

## Stage 7: WiFi Resilience (SC-5)

**Success Criterion SC-5:** WiFi survives a network drop and reconnects without a power cycle; heartbeat timeout triggers auto-stop

### Test A: Idle Disconnect/Reconnect

| # | Action | Expected Behavior |
|---|--------|-------------------|
| 1 | Ensure motors are stopped | No motion |
| 2 | Power cycle router (turn off, wait 5s, turn on) | -- |
| 3 | Observe serial log | Disconnect message, then reconnect messages |
| 4 | After reconnect, verify WiFi connected | Serial log shows new IP/RSSI |

### Test B: Moving Disconnect (Heartbeat Watchdog)

| # | Action | Expected Behavior |
|---|--------|-------------------|
| 1 | Start a motor via OSC (hold Companion button) | Motor running |
| 2 | Power cycle router (turn off) | -- |
| 3 | Observe motor | Heartbeat watchdog stops motors within ~500ms of lost connectivity |
| 4 | Wait for router to come back | ESP32 auto-reconnects |
| 5 | After reconnect, send OSC command | Motor responds again |

### Post-Reconnect mDNS

| # | Action | Expected Behavior |
|---|--------|-------------------|
| 6 | After WiFi reconnect, `ping ptzhead.local` | Still resolves (mDNS re-advertised on GOT_IP) |

### Pass Criteria
- [ ] Auto-reconnect after router power cycle
- [ ] Motors auto-stop within ~500ms on network loss (heartbeat watchdog)
- [ ] mDNS works again after reconnect

**Notes:**
_____________________________________________________________

---

## Stage 8: Heap Stability (Optional)

**Context:** CNMAT/OSC heap behavior flagged as a concern in Phase 2. Firmware logs free heap every 60 seconds.

### Steps

1. Note free heap value from serial log at boot (look for `[I] HEAP: free=XXXXX`)
2. Run an active session for 15-30 minutes:
   - Send OSC motion commands regularly
   - Change speed presets
   - Observe feedback in Companion
3. Note free heap after 15 minutes
4. Note free heap after 30 minutes

### Pass Criteria
- [ ] Heap stable (no significant downward trend over 30 minutes)

| Time | Free Heap |
|------|-----------|
| Boot | _________ |
| 15 min | _________ |
| 30 min | _________ |

**Notes:**
_____________________________________________________________

---

## Success Criteria Summary

| SC | Description | Stage(s) | Pass? |
|----|-------------|----------|-------|
| SC-1 | Firmware flashes and boots to Setup complete | Stage 1 | [ ] |
| SC-2 | Serial motor test: direction, speed, decel, auto-disable | Stage 2 | [ ] |
| SC-3 | OSC motion via Companion with <50ms latency + speed presets | Stage 3, 4 | [ ] |
| SC-4 | Feedback: moving state, preset, RSSI as integers | Stage 5 | [ ] |
| SC-5 | WiFi resilience: reconnect + heartbeat auto-stop | Stage 7 | [ ] |
| SC-6 | mDNS: ptzhead.local resolves, _osc._udp discoverable | Stage 6 | [ ] |
| SC-7 | Hardware issues captured as fixes or documented constraints | All stages | [ ] |

## Fixes Applied During Testing

| Stage | Issue | Fix | File |
|-------|-------|-----|------|
| | | | |

## Known Constraints

| Constraint | Stage | Notes |
|------------|-------|-------|
| | | |
