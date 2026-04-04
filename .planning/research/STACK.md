# Technology Stack

**Project:** PTZHead v2 -- OSC Overhaul
**Researched:** 2026-04-03
**Mode:** Ecosystem (brownfield overhaul)

## Recommended Stack

### Platform & Framework

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| espressif32 | `6.13.0` | PlatformIO platform | Latest stable (Feb 2025). Ships Arduino core v2.0.17 based on IDF v4.4.7. Drop-in replacement for current `6.10.0` -- same Arduino core, just newer IDF/toolchain support. | HIGH |
| Arduino framework | (bundled) | Application framework | Already in use, no reason to switch to ESP-IDF for this project scope. | HIGH |

**Key change:** Remove the Bluepad32-patched Arduino core entirely. The current `platform_packages` line that overrides `framework-arduinoespressif32` with the Bluepad32 fork must be deleted. The standard espressif32 platform already bundles the official Arduino core.

### OSC Library

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| CNMAT/OSC | `3.5.8` | OSC message encoding/decoding | The de facto standard OSC library for Arduino. Supports OSCMessage and OSCBundle, all four OSC data types (int32, float32, string, blob) plus timetags and booleans. Has ESP8266/ESP32 WiFiUDP examples in the repo. Well-known in the creative-tech community. 795 GitHub stars. | HIGH |

**Why CNMAT/OSC over alternatives:**

| Library | Verdict | Reason |
|---------|---------|--------|
| **CNMAT/OSC** | **USE THIS** | Battle-tested, full OSC spec support, bundles, ESP32 examples, active enough (last release Sep 2023, repo updated Dec 2024). Companion sends standard OSC -- this handles it cleanly. |
| MicroOsc | Skip | Lighter weight but less mature, fewer features (minimal error checking by design), smaller community. The RAM savings are negligible on ESP32 (520KB SRAM) vs the robustness tradeoff. |
| ArduinoOSC (hideakitai) | Skip | Overly abstracted with template-heavy API. Harder to debug on embedded. CNMAT is simpler and more transparent. |

### Network & Discovery

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| WiFiUDP | (bundled) | UDP transport for OSC | Part of ESP32 Arduino core. Standard UDP socket API. OSC messages are small (typically <100 bytes), well within single-packet limits. No fragmentation concerns. | HIGH |
| ESPmDNS | (bundled) | mDNS advertisement | Part of ESP32 Arduino core -- no extra dependency needed. `MDNS.begin("ptzhead")` makes the device discoverable as `ptzhead.local`. Supports service advertisement for zero-config. | HIGH |
| WiFiManager (tzapu) | `^2.0.17` | WiFi provisioning | Already in use and working. Captive portal for SSID/password setup. No reason to change. | HIGH |

### Motor Control

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| AccelStepper (waspinator) | `^1.64` | Stepper motor control | Already in use and proven. Drives 3 axes at up to 4000 sps with smooth accel/decel. The existing motion module wraps this well. | HIGH |

**Why NOT switch to FastAccelStepper:** FastAccelStepper uses hardware timers on ESP32 (MCPWM/PCNT peripherals) and can reach 200k steps/s. However, this project runs at 4000 sps max -- AccelStepper handles that trivially via polling in `loop()`. Switching would require rewriting the proven `ptz_motion` module for no functional benefit. FastAccelStepper is the right choice for CNC/3D-printer speeds; overkill here.

### Libraries to REMOVE

| Library | Current Version | Why Remove |
|---------|----------------|------------|
| WebSockets (links2004) | `^2.4.1` | Replaced by OSC over UDP. No WebSocket API needed. |
| ArduinoJson (bblanchon) | `^7.0.4` | Was used for WebSocket JSON messages. OSC is a binary protocol -- no JSON parsing needed. |
| Bluepad32 framework override | (platform_packages) | Gamepad support removed entirely. Switch to standard Arduino core. |

## platformio.ini -- Target Configuration

```ini
[env:esp32dev]
platform = espressif32@6.13.0
board = esp32dev
framework = arduino

monitor_speed = 115200
monitor_rts = 0
monitor_dtr = 0

; No platform_packages override -- use standard Arduino core

lib_deps =
  tzapu/WiFiManager@^2.0.17
  waspinator/AccelStepper@^1.64
  CNMAT/OSC@^3.5.8

build_flags =
  -DCORE_DEBUG_LEVEL=0
  -Os
  -ffunction-sections
  -fdata-sections
  -Wl,--gc-sections

build_unflags =
  -O2
  -flto
```

**Changes from current config:**
1. `platform` bumped from `6.10.0` to `6.13.0`
2. `platform_packages` block removed entirely (drops Bluepad32 fork)
3. `ArduinoJson` removed from `lib_deps`
4. `WebSockets` removed from `lib_deps`
5. `CNMAT/OSC@^3.5.8` added to `lib_deps`

## UDP Stack Considerations for ESP32

These are specific to running OSC over WiFi on ESP32 and directly affect reliability.

### WiFi Power Saving

The ESP32 defaults to `WIFI_PS_MIN_MODEM` which can cause intermittent packet delays (10-100ms jitter). For sub-50ms latency requirements:

```cpp
WiFi.setSleep(WIFI_PS_NONE);  // Disable WiFi power saving entirely
```

This increases power draw by ~20mA but eliminates WiFi sleep-induced latency. On a wired-power PTZ head, this is the right tradeoff.

**Confidence:** HIGH -- well-documented ESP32 behavior, multiple forum reports confirm.

### UDP Buffer and Packet Size

OSC messages for this project will be small:
- Velocity command: `/ptz/pan` + float arg = ~20 bytes
- Stop command: `/ptz/stop` = ~16 bytes
- Status feedback: `/ptz/status` + 3 floats + 3 ints = ~60 bytes

All well under the 1500-byte MTU. No fragmentation risk. The default ESP32 UDP receive buffer (configurable via menuconfig but defaults to ~1460 bytes) is sufficient.

### Unicast, Not Multicast

Use unicast UDP exclusively. ESP32 multicast reception is unreliable (4-30% packet loss reported across multiple sources). Companion sends to a specific IP:port, and the PTZ head responds to the sender's IP:port. This is natural unicast.

**Confidence:** HIGH -- unicast is the standard pattern for Companion OSC connections.

### Receive Loop Frequency

`WiFiUDP.parsePacket()` must be called frequently enough to drain the receive buffer before it overflows. At Companion's typical send rate (~10-20 messages/sec for held buttons), calling `parsePacket()` every `loop()` iteration is sufficient. AccelStepper's `run()` already demands a tight loop, so this aligns naturally.

## Companion OSC Integration Notes

Bitfocus Companion's generic-osc module supports:
- Sending: int, float, string, multi-arg, boolean, blob (base64/hex)
- Receiving: int, float, boolean, multi-arg, no-arg messages
- Variables: can expose received OSC values as Companion variables
- Transport: UDP (default) or TCP

**Design implication:** The PTZ head should send status back to Companion so button LEDs can reflect state (e.g., "moving" indicator). Companion can receive OSC messages and use them for feedback. This is standard OSC bidirectional communication -- send status to the IP:port that last sent a command.

## Alternatives Considered

| Category | Recommended | Alternative | Why Not |
|----------|-------------|-------------|---------|
| OSC library | CNMAT/OSC | MicroOsc | Less mature, minimal error checking, smaller community. RAM savings irrelevant on ESP32. |
| OSC library | CNMAT/OSC | ArduinoOSC (hideakitai) | Over-engineered template API, harder to debug. |
| Stepper lib | AccelStepper | FastAccelStepper | Hardware timer approach is overkill at 4000 sps. Would require rewriting proven motion module. |
| Protocol | OSC/UDP | OSC/TCP | UDP is standard for real-time control. TCP adds latency from retransmission. Companion defaults to UDP. |
| mDNS | ESPmDNS (bundled) | External mDNS lib | No need -- ESPmDNS ships with the Arduino core and works. |
| Framework | Arduino | ESP-IDF | Massive rewrite for no benefit. Arduino abstractions are fine for this project's complexity. |

## Version Verification Summary

| Library | Claimed Version | Verification Source | Status |
|---------|----------------|--------------------|----|
| espressif32 | 6.13.0 | [GitHub releases](https://github.com/platformio/platform-espressif32/releases) | Verified -- released Feb 26, 2025 |
| CNMAT/OSC | 3.5.8 | [GitHub releases](https://github.com/CNMAT/OSC/releases), [library.json](https://github.com/CNMAT/OSC/blob/master/library.json) | Verified -- released Sep 4, 2023 |
| WiFiManager | 2.0.17 | [GitHub](https://github.com/tzapu/WiFiManager) | Verified -- already in use |
| AccelStepper | 1.64+ | [PlatformIO registry](https://registry.platformio.org/libraries/waspinator/AccelStepper) | Verified -- 1.65/1.66 available, ^1.64 will resolve to latest |
| ESPmDNS | (bundled) | [arduino-esp32 repo](https://github.com/espressif/arduino-esp32/blob/master/libraries/ESPmDNS/src/ESPmDNS.h) | Verified -- ships with Arduino core |

## Sources

- [CNMAT/OSC GitHub](https://github.com/CNMAT/OSC) -- library source, examples, releases
- [CNMAT/OSC ESP32 issue #59](https://github.com/CNMAT/OSC/issues/59) -- ESP32 compatibility (resolved 2018)
- [CNMAT/OSC PlatformIO registry](https://registry.platformio.org/libraries/cnmat/OSC)
- [espressif32 releases](https://github.com/platformio/platform-espressif32/releases)
- [ESPmDNS in arduino-esp32](https://github.com/espressif/arduino-esp32/blob/master/libraries/ESPmDNS/src/ESPmDNS.h)
- [Companion generic-osc module](https://github.com/bitfocus/companion-module-generic-osc/blob/master/companion/HELP.md)
- [FastAccelStepper](https://github.com/gin66/FastAccelStepper) -- evaluated and rejected for this use case
- [MicroOsc](https://github.com/thomasfredericks/MicroOsc) -- evaluated and rejected
- [ESP32 WiFiUDP packet bunching](https://forum.arduino.cc/t/esp32-wifi-udp-bunching-packets/1162055)
- [ESP32 UDP performance issues](https://github.com/espressif/arduino-esp32/issues/1317)
- [ESP32 multicast packet loss](https://github.com/espressif/arduino-esp32/issues/8652)
- [ESP32 OSC receiver example](https://madskjeldgaard.dk/old-blog/esp32-simple-osc-receiver/)
