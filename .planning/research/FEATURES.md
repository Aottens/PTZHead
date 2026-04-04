# Feature Research

**Domain:** OSC-controlled PTZ camera head (ESP32) with Bitfocus Companion integration
**Researched:** 2026-04-03
**Confidence:** HIGH (well-understood domain, clear requirements from PROJECT.md, established Companion module patterns)

## Feature Landscape

### Table Stakes (Users Expect These)

Features that any OSC-controlled PTZ device must have to be considered functional. Missing any of these means the device is broken for its intended use case.

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| Per-axis velocity control via OSC | Core interaction model: hold button = move, release = stop. Every Companion PTZ module has directional movement commands. | MEDIUM | Normalized float (-1.0 to 1.0) per axis. Existing `setVelocity()` already handles this, need OSC parsing layer on top. |
| All-stop command | Safety requirement. Operator must be able to halt all motion instantly. Standard in every PTZ module (Panasonic, PTZOptics, etc.). | LOW | Single OSC message `/ptz/stop` that calls existing `stop()`. Trivial. |
| Per-axis stop | Companion PTZ modules offer per-axis stop. Operator may want to stop pan but keep tilt moving. | LOW | `/ptz/pan/stop`, `/ptz/tilt/stop`, `/ptz/zoom/stop`. |
| Smooth acceleration/deceleration | Jerky motion is immediately obvious on camera. AccelStepper already provides this. Must be preserved through the OSC transition. | LOW | Already implemented in `ptz_motion`. Zero new work, just don't break it. |
| WiFi connectivity with captive portal provisioning | Zero-config setup without recompiling firmware. Already implemented via WiFiManager. | LOW | Already working. Preserve as-is. |
| mDNS advertisement | Companion users expect to type `ptzhead.local` instead of hunting for IP addresses. Standard for network AV devices. | LOW | ESP32 Arduino has built-in mDNS (`ESPmDNS.h`). Advertise an `_osc._udp` service. |
| OSC feedback: moving state | Companion needs to know if the head is moving to light up direction buttons. The generic OSC module can listen for incoming integer/boolean values and apply feedbacks. | MEDIUM | Send `/ptz/moving` (bool) and per-axis `/ptz/pan/moving` (bool) at a reasonable interval. |
| OSC feedback: connection/alive status | Companion generic OSC module can detect if a device stops responding. Heartbeat/keepalive pattern. | LOW | Periodic `/ptz/heartbeat` or simply rely on the status broadcast interval. |
| Motor idle shutdown | Stepper drivers draw current and heat up when energized but idle. Already implemented. | LOW | Already working. Preserve existing idle timeout logic. |
| Speed presets (switchable speed/accel profiles) | Operators need different speeds for different shots: slow creep for interviews, fast snap for event coverage. Panasonic module has "Set Pan/Tilt Speed" and speed up/down actions. | MEDIUM | Define 3-5 named presets (e.g., slow, medium, fast, snap) that set maxSpeed and acceleration values per axis. Switch via `/ptz/speed/preset i`. |

### Differentiators (Competitive Advantage)

Features that make this PTZ head notably better than a generic "hook up OSC to steppers" project. Not required for launch, but valuable.

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| Diagonal movement commands | Companion PTZ modules (Panasonic, Canon) offer UpLeft, UpRight, DownLeft, DownRight as single actions. More convenient than requiring two simultaneous button presses. | LOW | Combination velocity commands: `/ptz/move/upleft` etc. Sugar on top of `setVelocity()`. |
| Per-axis speed override via OSC argument | Instead of only preset speeds, allow `/ptz/pan f:0.5` where the float argument IS the normalized speed. Gives Companion faders/encoders direct control. | LOW | Already how `setVelocity()` works internally. Just expose the float argument directly. |
| OSC feedback: current speed preset | Companion can highlight which speed preset is active on the StreamDeck. Nice visual confirmation. | LOW | Send `/ptz/speed/current i:2` when preset changes. |
| Rate-limited status broadcast | Periodic broadcast of position, speed, and state at a configurable interval. Useful for Companion variable display and debugging. | LOW | Existing `broadcastStatus` pattern, convert from WebSocket JSON to OSC bundles. |
| WiFi RSSI feedback | Show signal strength on StreamDeck. Helps diagnose flaky behavior before it becomes a problem during a show. | LOW | Already read in `loop()`. Send as `/ptz/wifi/rssi i:-65`. |
| Configurable OSC ports via captive portal | Let operator set send/receive ports during WiFi setup instead of recompiling. | MEDIUM | Extend WiFiManager custom parameters. Store in EEPROM/NVS. |
| Emergency all-stop on connection loss | If no OSC message received within timeout, stop all motion. Prevents runaway head if WiFi drops. | LOW | Already have `kAppHeartbeatTimeoutMs` pattern. Reuse for OSC source timeout. |
| Invert axis via OSC | Flip pan or tilt direction without reflashing. Useful when camera is mounted upside-down or mirrored. | LOW | `/ptz/config/invert/pan i:1`. Store in NVS for persistence. |

### Anti-Features (Commonly Requested, Often Problematic)

Features that seem useful but would add complexity without matching the project's constraints or goals.

| Feature | Why Requested | Why Problematic | Alternative |
|---------|---------------|-----------------|-------------|
| Position presets (save/recall) | Every commercial PTZ has presets. Panasonic module supports 100 presets. | No encoders on this hardware -- stepper position is only tracked from power-on, drifts on stalls, and resets on reboot. Presets would be unreliable and give false confidence. PROJECT.md explicitly defers this. | Defer until encoders are added. Document clearly as future hardware upgrade. |
| Web UI dashboard | Tempting to add a status page. Existing gamepad firmware had WebSocket for this. | Companion IS the dashboard. A web UI duplicates functionality, wastes ESP32 RAM, and creates a second interface to maintain. PROJECT.md explicitly excludes this. | Use Companion variables and feedbacks for all status display. Serial logging for debugging. |
| Multi-client OSC arbitration | Multiple Companion instances or other OSC sources controlling the head. | Ownership arbitration was already built (`ptz_owner`) and is being removed for good reason -- adds complexity for a use case that doesn't exist. Single operator assumed. | Single OSC source. If needed later, add a simple "last writer wins" with timeout. |
| Custom OSC namespace configuration | Let users redefine OSC addresses. | Massively increases complexity. Every message needs a lookup table. Makes documentation useless. No Companion PTZ module does this. | Ship a well-documented fixed namespace. It's firmware, not a framework. |
| TCP OSC transport | OSC over TCP for guaranteed delivery. | UDP is the standard for real-time control (low latency, no head-of-line blocking). Companion's generic OSC module uses UDP. TCP adds latency and complexity on ESP32. | Stay with UDP. The hold-to-move pattern is inherently self-correcting (next message fixes any lost one). |
| OTA firmware updates | Update firmware over WiFi. | Significant flash partition complexity, security concerns, and ESP32 RAM pressure during OTA. Overkill for a single device updated infrequently. | USB flashing via PlatformIO. Standard for development hardware. |
| Absolute position commands | `/ptz/pan/goto f:1500` to move to a step position. | Without encoders, absolute positions are meaningless after a power cycle or stall. Misleading API. | Velocity control only. Add absolute positioning when encoders exist. |

## Feature Dependencies

```
[WiFi connectivity]
    |
    +--requires--> [mDNS advertisement]
    |
    +--requires--> [OSC message parsing]
                       |
                       +--requires--> [Per-axis velocity control]
                       |                   |
                       |                   +--enhances--> [Diagonal movement commands]
                       |                   +--enhances--> [Per-axis speed override]
                       |
                       +--requires--> [All-stop command]
                       |
                       +--requires--> [Speed presets]
                       |                   |
                       |                   +--enhances--> [OSC feedback: current speed preset]
                       |
                       +--requires--> [OSC feedback: moving state]
                       |
                       +--requires--> [Emergency stop on connection loss]

[Motor idle shutdown] (independent, already exists)

[Configurable OSC ports] --requires--> [WiFi captive portal parameters]
```

### Dependency Notes

- **All OSC features require OSC message parsing:** The OSC library and UDP listener must be working before any command can be processed. This is the foundation layer.
- **mDNS requires WiFi:** Obviously. But mDNS setup is trivial and should happen right after WiFi connects.
- **Speed presets enable speed feedback:** Can't report current preset if presets don't exist yet.
- **Connection loss detection requires OSC traffic:** The timeout pattern only works once OSC messages are flowing.
- **Diagonal commands enhance velocity control:** Pure sugar -- they compose existing per-axis velocity into convenience commands.

## MVP Definition

### Launch With (v1)

Minimum viable firmware that replaces the gamepad+WebSocket architecture with working OSC control from Companion.

- [ ] OSC UDP listener on configurable port -- foundation for everything
- [ ] Per-axis velocity control (`/ptz/pan f`, `/ptz/tilt f`, `/ptz/zoom f`) -- core interaction
- [ ] All-stop (`/ptz/stop`) and per-axis stop -- safety requirement
- [ ] Speed presets switchable via OSC (`/ptz/speed/preset i`) -- operator workflow
- [ ] OSC feedback: moving state (per-axis booleans) -- Companion button feedback
- [ ] mDNS advertisement (`ptzhead.local`, `_osc._udp` service) -- zero-config discovery
- [ ] Connection loss timeout with auto-stop -- safety net
- [ ] Remove gamepad, WebSocket, and owner modules -- clean architecture

### Add After Validation (v1.x)

Features to add once the core OSC control loop is proven stable and responsive.

- [ ] Diagonal movement convenience commands -- when operators request it
- [ ] WiFi RSSI feedback -- when debugging network issues in the field
- [ ] Configurable OSC ports via captive portal -- when deploying to environments with port conflicts
- [ ] Axis inversion via OSC with NVS persistence -- when mounting configurations vary
- [ ] Status broadcast bundle (position, speed, all state in one periodic message) -- when Companion page needs richer display

### Future Consideration (v2+)

Features that require hardware changes or significantly expanded scope.

- [ ] Position presets -- defer until encoders are added to hardware
- [ ] Absolute positioning -- defer until encoders exist
- [ ] Multi-device sync -- defer until there are multiple PTZ heads to coordinate
- [ ] Custom Companion module (instead of generic OSC) -- defer until the OSC namespace is stable and proven

## Feature Prioritization Matrix

| Feature | User Value | Implementation Cost | Priority |
|---------|------------|---------------------|----------|
| Per-axis velocity control | HIGH | MEDIUM | P1 |
| All-stop / per-axis stop | HIGH | LOW | P1 |
| Speed presets | HIGH | MEDIUM | P1 |
| OSC moving state feedback | HIGH | LOW | P1 |
| mDNS advertisement | HIGH | LOW | P1 |
| Connection loss auto-stop | HIGH | LOW | P1 |
| Remove legacy modules | HIGH | LOW | P1 |
| Diagonal movement commands | MEDIUM | LOW | P2 |
| Per-axis speed override (float arg) | MEDIUM | LOW | P2 |
| Speed preset feedback | MEDIUM | LOW | P2 |
| WiFi RSSI feedback | LOW | LOW | P2 |
| Configurable OSC ports | MEDIUM | MEDIUM | P2 |
| Axis inversion via OSC | MEDIUM | LOW | P2 |
| Status broadcast bundle | MEDIUM | LOW | P2 |

**Priority key:**
- P1: Must have for launch -- the head is non-functional without these
- P2: Should have, add when core is stable
- P3: Nice to have, future consideration (hardware-dependent features)

## Companion Integration Patterns

Based on analysis of Panasonic PTZ, PTZOptics, and generic OSC Companion modules:

| Pattern | Commercial PTZ Modules | Our Approach |
|---------|----------------------|--------------|
| Directional movement (8-way) | All modules offer Up/Down/Left/Right + diagonals as discrete actions | Support both discrete direction commands AND raw float velocity for flexibility |
| Speed control | Set speed + speed up/down actions. Panasonic has independent pan/tilt speed. | Speed presets (named profiles) switchable via single OSC command. Simpler for StreamDeck workflow. |
| Status feedback | Variables for model, firmware, power state, tally, position | Feedback for moving state (per-axis bool), active speed preset, WiFi RSSI. Keep it minimal. |
| Connection status | Module shows "connected" / "disconnected" in Companion | Periodic heartbeat message. Generic OSC module can detect absence. |
| Presets | Save/recall numbered presets (up to 100 on Panasonic) | Explicitly not supported (no encoders). Document clearly. |

## Proposed OSC Namespace

Based on conventions observed in PTZ controllers and OSC best practices:

```
# Movement (velocity control, normalized -1.0 to 1.0)
/ptz/pan       f:[-1.0..1.0]    # pan velocity
/ptz/tilt      f:[-1.0..1.0]    # tilt velocity
/ptz/zoom      f:[-1.0..1.0]    # zoom velocity

# Stop
/ptz/stop                        # all-stop
/ptz/pan/stop                    # stop pan only
/ptz/tilt/stop                   # stop tilt only
/ptz/zoom/stop                   # stop zoom only

# Speed presets
/ptz/speed/preset  i:[0..N]     # switch active preset
/ptz/speed/get                   # request current preset

# Feedback (device -> Companion)
/ptz/status/moving    i:[0|1]   # any axis moving
/ptz/status/pan       i:[0|1]   # pan moving
/ptz/status/tilt      i:[0|1]   # tilt moving
/ptz/status/zoom      i:[0|1]   # zoom moving
/ptz/status/speed     i:[0..N]  # active speed preset index
/ptz/status/wifi      i:[-100..0]  # RSSI dBm
/ptz/heartbeat        i:[ms]    # uptime heartbeat
```

**Design rationale:**
- Integer feedback (not float/bool) because Companion generic OSC feedbacks work most reliably with integers
- Flat namespace under `/ptz/` prefix -- simple, no ambiguity
- Movement uses floats (continuous control), status uses integers (discrete states)
- Status messages use `/ptz/status/` sub-namespace to clearly separate commands from feedback

## Sources

- [Bitfocus Companion generic OSC module](https://github.com/bitfocus/companion-module-generic-osc) -- feedback capabilities and limitations
- [Companion generic OSC HELP.md](https://github.com/bitfocus/companion-module-generic-osc/blob/master/companion/HELP.md) -- available actions, feedbacks, variables
- [Companion Panasonic PTZ module](https://github.com/bitfocus/companion-module-panasonic-ptz) -- reference for PTZ module feature set
- [Panasonic PTZ HELP.md](https://github.com/bitfocus/companion-module-panasonic-ptz/blob/master/companion/HELP.md) -- actions, feedbacks, variables for commercial PTZ
- [OSC Feedback Support issue #13](https://github.com/bitfocus/companion-module-generic-osc/issues/13) -- feedback limitations in generic module
- [Incoming value -> custom variable issue #45](https://github.com/bitfocus/companion-module-generic-osc/issues/45) -- workarounds for OSC variable mapping
- [ESP32 mDNS documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/mdns.html) -- mDNS service advertisement
- [Companion Feedbacks documentation](https://companion.free/for-developers/module-development/connection-basics/feedbacks/) -- how feedbacks work in modules
- Existing codebase analysis: `ptz_motion.h`, `ptz_config.h`, `main.cpp`

---
*Feature research for: ESP32 OSC PTZ camera head with Bitfocus Companion integration*
*Researched: 2026-04-03*
