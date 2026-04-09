# PTZHead v2 — OSC Camera Head Controller

ESP32-based Pan-Tilt-Zoom camera head controller driven by OSC over WiFi. Designed to be controlled from a StreamDeck via [Bitfocus Companion](https://bitfocus.io/companion).

## Features

- **3-axis stepper control** (pan, tilt, zoom) with smooth acceleration via FastAccelStepper
- **OSC over UDP** — hold a button, head moves, release and it stops
- **3 speed presets** (slow / medium / fast) switchable via OSC
- **Live feedback** — moving state, active preset, WiFi RSSI sent back to Companion
- **mDNS discovery** — finds itself on the network as `ptzhead.local`
- **WiFi resilience** — auto-reconnects after network drops, heartbeat watchdog auto-stops motors
- **StreamDeck ready** — simple press/release buttons, no repeat loops needed

## Build & Upload

```sh
pio run                    # compile
pio run --target upload    # flash to ESP32
pio device monitor         # serial monitor (115200 baud)
```

## WiFi Setup

On first boot (or after `WIFI RESET`), the ESP32 starts a captive portal:
1. Connect to the **"PTZHead Setup"** WiFi network
2. Enter your network credentials
3. Device reconnects and is ready

## OSC Commands

| Address | Type | Value | Description |
|---------|------|-------|-------------|
| `/ptz/pan` | float | -1.0 to 1.0 | Pan velocity (0.0 = stop) |
| `/ptz/tilt` | float | -1.0 to 1.0 | Tilt velocity (0.0 = stop) |
| `/ptz/zoom` | float | -1.0 to 1.0 | Zoom velocity (0.0 = stop) |
| `/ptz/stop` | — | — | Stop all axes |
| `/ptz/pan/stop` | — | — | Stop pan only |
| `/ptz/tilt/stop` | — | — | Stop tilt only |
| `/ptz/zoom/stop` | — | — | Stop zoom only |
| `/ptz/speed/preset` | int | 0 / 1 / 2 | Slow / Medium / Fast |

**Default port:** `8000` (UDP)

## OSC Feedback

Sent back to the last OSC sender automatically:

| Address | Type | Description |
|---------|------|-------------|
| `/ptz/status/pan/moving` | int | 1 = moving, 0 = stopped |
| `/ptz/status/tilt/moving` | int | 1 = moving, 0 = stopped |
| `/ptz/status/zoom/moving` | int | 1 = moving, 0 = stopped |
| `/ptz/status/preset` | int | Active speed preset index |
| `/ptz/status/rssi` | int | WiFi signal strength (dBm) |

## Serial Commands

For debugging without OSC:

```
PAN 1.0       # pan at full speed
TILT -0.5     # tilt at half speed, reverse
ZOOM 1.0      # zoom in
STOP          # stop all axes
WIFI RESET    # clear credentials, reopen captive portal
```

## Companion Setup

1. Add a **Generic: OSC** connection → `ptzhead.local` (or IP) port `8000` UDP
2. Motion buttons: **Press** sends velocity (e.g. `/ptz/pan` = `1.0`), **Release** sends `0.0`
3. Speed buttons: **Press** sends `/ptz/speed/preset` with `0`, `1`, or `2`
4. STOP button: **Press** sends `/ptz/stop`

No repeat intervals needed — the firmware has a 5-second heartbeat timeout as a safety net.

## Configuration

Key constants in `src/ptz_config.h`:

| Constant | Default | Description |
|----------|---------|-------------|
| `kPanMaxSps` | 4000 | Max pan speed (steps/sec) |
| `kTiltMaxSps` | 4000 | Max tilt speed (steps/sec) |
| `kZoomMaxSps` | 4000 | Max zoom speed (steps/sec) |
| `kPanAccel` | 20000 | Pan acceleration (steps/sec²) |
| `kInvertPan` | true | Flip pan direction |
| `kHeartbeatTimeoutMs` | 5000 | Auto-stop timeout (ms) |
| `kOscPort` | 8000 | OSC UDP listen port |

## Hardware

- ESP32 dev board
- 3x stepper motor drivers (active-low enable)
- Pin assignments in `src/ptz_config.h`

## License

MIT
