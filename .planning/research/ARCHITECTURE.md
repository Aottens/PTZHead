# Architecture Patterns

**Domain:** ESP32 OSC-controlled PTZ camera head
**Researched:** 2026-04-03

## Recommended Architecture

The firmware keeps the existing pattern: loosely coupled modules in the `ptz::` namespace, orchestrated by a thin `main.cpp` loop. This pattern works well on ESP32 -- flat, no dynamic dispatch, easy to reason about timing. The overhaul replaces the input layer (gamepad/WebSocket -> OSC) and removes the ownership arbitrator (single source now).

### Component Diagram

```
                    WiFi (UDP)
                       |
              +--------+--------+
              |                 |
        [OSC Input]       [OSC Output]
         (ptz_osc)         (ptz_osc)
              |                 ^
              v                 |
        [Dispatcher]      [Status Poller]
              |                 ^
              v                 |
        [Motion]           [Motion State]
        (ptz_motion)       (ptz_motion)
              |
              v
        [AccelStepper x3]
              |
              v
        [Stepper Drivers]


  Side modules:
  [WiFiManager]  (ptz_wifi)  -- provisioning only
  [ESPmDNS]      (ptz_mdns)  -- advertisement only
  [Logger]       (ptz_log)   -- serial output
  [Config]       (ptz_config) -- constants
```

### Component Boundaries

| Component | Responsibility | Communicates With | Kept/New |
|-----------|---------------|-------------------|----------|
| `ptz_osc` | Parse incoming OSC, send outgoing OSC status | Motion (commands), Config (speed presets), WiFiUDP | **New** |
| `ptz_motion` | 3-axis stepper velocity control, acceleration, idle shutdown | AccelStepper (hardware), called by main loop | **Kept** (minor refactor) |
| `ptz_wifi` | WiFiManager captive portal provisioning, connection management | WiFi hardware, Serial (commands) | **Kept** as-is |
| `ptz_mdns` | mDNS hostname + service advertisement | ESPmDNS, runs after WiFi connects | **New** (tiny) |
| `ptz_config` | All constants: pins, speeds, ports, OSC addresses | Read by everyone, writes to nobody | **Kept** (extended) |
| `ptz_log` | Rate-limited serial logging macros | Serial hardware | **Kept** as-is |
| `main.cpp` | Setup sequencing, main loop orchestration, idle timeout | All modules | **Refactored** (simplified) |

### Modules Removed

| Module | Reason |
|--------|--------|
| `ptz_gamepad` | Bluepad32 removed, no gamepad input |
| `ptz_ws` | WebSocket replaced by OSC |
| `ptz_owner` | Single input source (OSC), no arbitration needed |

## Data Flow

### Inbound: OSC Command to Motor Movement

```
1. WiFiUDP receives UDP packet on port 8000
2. ptz_osc::update() reads packet, parses as OSCMessage
3. CNMAT OSC library routes by address pattern:
   /ptz/pan     -> setVelocity(pan, 0, 0)
   /ptz/tilt    -> setVelocity(0, tilt, 0)
   /ptz/zoom    -> setVelocity(0, 0, zoom)
   /ptz/move    -> setVelocity(pan, tilt, zoom)  [combined]
   /ptz/stop    -> stop()
   /ptz/speed   -> switch speed preset
4. ptz_motion::update(dt) applies slew-rate limiting, updates targets
5. ptz_motion::run() calls AccelStepper::run() for each axis
```

### Outbound: Status Feedback to Companion

```
1. Main loop hits status interval (every 50ms)
2. ptz_osc::sendStatus() builds OSCBundle:
   /ptz/status/pan      float (current position)
   /ptz/status/tilt     float (current position)
   /ptz/status/zoom     float (current position)
   /ptz/status/moving   int (0 or 1)
   /ptz/status/speed    int (current preset index)
3. Sends bundle via UDP to Companion's IP:port
```

### Key Data Flow Decisions

**Per-axis vs combined velocity commands:** Support both. Companion buttons are per-axis (one button = pan left), but a combined `/ptz/move` endpoint allows a single message to set all three. Per-axis commands only modify that axis, leaving others unchanged. This matches how StreamDeck buttons work -- each button controls one direction.

**Companion IP discovery:** Two strategies, both needed:
1. **Static config:** Companion IP/port stored in `ptz_config.h` as defaults. Works for fixed setups.
2. **Reply-to-sender:** When an OSC message arrives, cache the sender's IP from the UDP packet. Send status feedback to the last-seen sender. This is the zero-config path and should be the primary mechanism.

**Status rate:** 50ms interval (20Hz) matches the existing WebSocket broadcast rate. Sufficient for Companion button LED feedback without flooding the network.

## OSC Address Namespace

```
INBOUND (Companion -> PTZHead):
  /ptz/pan          float [-1.0 .. 1.0]    Pan velocity (normalized)
  /ptz/tilt         float [-1.0 .. 1.0]    Tilt velocity (normalized)
  /ptz/zoom         float [-1.0 .. 1.0]    Zoom velocity (normalized)
  /ptz/move         float, float, float     Combined pan, tilt, zoom
  /ptz/stop                                 Stop all axes (no args)
  /ptz/stop/pan                             Stop pan only
  /ptz/stop/tilt                            Stop tilt only
  /ptz/stop/zoom                            Stop zoom only
  /ptz/speed        int [0..N]              Switch speed preset

OUTBOUND (PTZHead -> Companion):
  /ptz/status/pan   float                   Current pan position
  /ptz/status/tilt  float                   Current tilt position
  /ptz/status/zoom  float                   Current zoom position
  /ptz/status/moving int                    1 if any axis moving
  /ptz/status/speed int                     Current speed preset index
  /ptz/status/wifi  int                     RSSI value
```

**Why this namespace:**
- `/ptz/` prefix groups all messages, avoids collisions
- Flat under `/ptz/` for simple Companion action configuration
- Status under `/ptz/status/` keeps feedback separated from commands
- Normalized floats (-1.0 to 1.0) for velocity match the existing `setVelocity()` API directly

## Patterns to Follow

### Pattern 1: Reply-to-Sender for Feedback Target

**What:** Instead of hardcoding Companion's IP, capture the source IP/port from the first incoming UDP packet and use it as the feedback destination.

**When:** Always, as the primary feedback mechanism.

**Why:** Zero-config. Companion connects, PTZHead automatically knows where to send status. Works if Companion's IP changes. Degrades gracefully -- if no OSC received yet, no status sent (nothing to send to).

```cpp
// In ptz_osc.cpp
IPAddress feedbackIp_;
uint16_t feedbackPort_ = 0;
bool hasFeedbackTarget_ = false;

void PtzOsc::update() {
    int packetSize = udp_.parsePacket();
    if (packetSize > 0) {
        feedbackIp_ = udp_.remoteIP();
        feedbackPort_ = udp_.remotePort();
        hasFeedbackTarget_ = true;
        // ... parse OSC message
    }
}

void PtzOsc::sendStatus(const ptz::MotionState& state, bool moving, int rssi) {
    if (!hasFeedbackTarget_) return;
    // ... build and send OSCBundle to feedbackIp_:feedbackPort_
}
```

### Pattern 2: Dispatch Table for OSC Routing

**What:** Use CNMAT OSC library's `route()` method with callbacks to map addresses to actions. Keep the dispatch logic inside `ptz_osc`, not scattered across modules.

**When:** For all incoming OSC message handling.

```cpp
void PtzOsc::handleMessage(OSCMessage& msg) {
    msg.route("/ptz/pan",   [this](OSCMessage& m, int offset) { onPan(m); });
    msg.route("/ptz/tilt",  [this](OSCMessage& m, int offset) { onTilt(m); });
    msg.route("/ptz/zoom",  [this](OSCMessage& m, int offset) { onZoom(m); });
    msg.route("/ptz/move",  [this](OSCMessage& m, int offset) { onMove(m); });
    msg.route("/ptz/stop",  [this](OSCMessage& m, int offset) { onStop(m); });
    msg.route("/ptz/speed", [this](OSCMessage& m, int offset) { onSpeed(m); });
}
```

**Note:** CNMAT `route()` does partial prefix matching on `/` boundaries. For the stop sub-commands (`/ptz/stop/pan`), use `dispatch()` for exact matching, or route `/ptz/stop` and inspect the remaining offset to determine which axis.

### Pattern 3: Speed Presets as Config Arrays

**What:** Define speed presets as arrays in `ptz_config.h`. OSC selects by index. No runtime configuration needed.

**When:** For speed/acceleration control via OSC.

```cpp
// ptz_config.h
struct SpeedPreset {
    float panMaxSps;
    float tiltMaxSps;
    float zoomMaxSps;
    float panAccel;
    float tiltAccel;
    float zoomAccel;
};

constexpr SpeedPreset kSpeedPresets[] = {
    {2000.0f, 2000.0f, 2000.0f, 10000.0f, 10000.0f, 8000.0f},  // Slow
    {4000.0f, 4000.0f, 4000.0f, 20000.0f, 20000.0f, 15000.0f},  // Normal
    {6000.0f, 6000.0f, 6000.0f, 30000.0f, 30000.0f, 20000.0f},  // Fast
};
constexpr uint8_t kDefaultPresetIndex = 1;
```

### Pattern 4: Main Loop Stays Thin

**What:** `main.cpp` calls `update()` on each module in order. No business logic in main.

**Why:** The current main.cpp is already close to this, but has gamepad/owner logic mixed in. With a single input source, main becomes much simpler.

```cpp
void loop() {
    const uint32_t nowMs = millis();
    float dt = computeDt();

    osc.update();           // Parse incoming OSC, apply commands to motion
    handleSerialCommands(); // WiFi reset trigger
    motion.update(dt);      // Slew-rate limiting, target updates
    motion.run();           // Step generation

    updateIdleTimeout(nowMs);
    updateStatusBroadcast(nowMs);
}
```

## Anti-Patterns to Avoid

### Anti-Pattern 1: OSC Parsing in Main Loop

**What:** Putting OSC packet reading, parsing, and dispatch logic directly in `loop()`.

**Why bad:** Couples protocol details to orchestration. Makes it impossible to test or swap protocols. The current WebSocket handler already made this mistake partially (main.cpp knows about owner states driven by WS events).

**Instead:** `ptz_osc` encapsulates all protocol handling. Main calls `osc.update()`, which internally reads UDP, parses, and calls motion methods via a stored pointer.

### Anti-Pattern 2: Bidirectional Module Dependencies

**What:** Motion module knowing about OSC, OSC module knowing about WiFi internals.

**Why bad:** Creates coupling that makes modules hard to modify independently.

**Instead:** One-way dependencies only. OSC depends on Motion (calls setVelocity/stop). Motion depends on nothing except AccelStepper and config. Main orchestrates the status broadcast by reading motion state and passing it to OSC's send method.

### Anti-Pattern 3: Dynamic Memory for OSC Messages

**What:** Using `new`/`malloc` for OSC message buffers on every packet.

**Why bad:** Heap fragmentation on ESP32, especially at 20Hz status broadcast rate. Eventually crashes.

**Instead:** Static buffers. CNMAT OSC library uses stack allocation for messages. Keep a fixed-size UDP receive buffer (e.g., 256 bytes -- OSC messages for this use case are tiny). Reuse the same buffer each loop iteration.

### Anti-Pattern 4: Blocking WiFi Reconnect in Loop

**What:** Adding WiFi reconnection logic that blocks the main loop.

**Why bad:** AccelStepper needs `run()` called frequently for smooth motion. A blocking reconnect stalls step generation, causing jerky movement or missed steps.

**Instead:** WiFiManager handles reconnection non-blocking. If WiFi drops, OSC stops arriving but motion continues decelerating to a stop naturally (no new velocity commands = velocity commands stay at zero).

## PtzOsc Module Design (Confidence: HIGH)

This is the single new module that replaces both `ptz_ws` and `ptz_owner`.

```cpp
// ptz_osc.h
#pragma once

#include <WiFiUdp.h>
#include "ptz_motion.h"

namespace ptz {

class PtzOsc {
 public:
    void begin(PtzMotion* motion, uint16_t listenPort);
    void update();       // Read + parse incoming OSC
    void sendStatus(const MotionState& state, bool moving,
                    uint8_t speedPreset, int wifiRssi);

 private:
    void handleMessage(/* OSCMessage& */);
    void onPan(/* OSCMessage& */);
    void onTilt(/* OSCMessage& */);
    void onZoom(/* OSCMessage& */);
    void onMove(/* OSCMessage& */);
    void onStop(/* OSCMessage& */);
    void onSpeed(/* OSCMessage& */);

    WiFiUDP udp_;
    PtzMotion* motion_ = nullptr;

    IPAddress feedbackIp_;
    uint16_t feedbackPort_ = 0;
    bool hasFeedbackTarget_ = false;

    uint8_t rxBuffer_[256];  // Static receive buffer
};

} // namespace ptz
```

**Key design decisions:**
- Takes a pointer to `PtzMotion` in `begin()`, same pattern as the existing `PtzWebSocket`.
- Owns the `WiFiUDP` instance. One UDP socket, one port.
- Static receive buffer -- no heap allocation per packet.
- Feedback target discovered from incoming packets, not configured.
- `sendStatus()` called by main loop with data it already has, keeping OSC module unaware of timing.

## mDNS Module Design (Confidence: HIGH)

Minimal wrapper. Could even be inline in `main.cpp`, but a dedicated module keeps setup() clean and allows advertising the OSC service properly.

```cpp
// ptz_mdns.h
#pragma once

namespace ptz {

class PtzMdns {
 public:
    void begin(const char* hostname, uint16_t oscPort);
};

} // namespace ptz

// ptz_mdns.cpp
#include <ESPmDNS.h>

void ptz::PtzMdns::begin(const char* hostname, uint16_t oscPort) {
    MDNS.begin(hostname);                    // e.g., "ptzhead"
    MDNS.addService("osc", "udp", oscPort);  // Advertise OSC service
}
```

**Why a separate module:** `MDNS.addService()` is required -- without it, mDNS stops responding after a few minutes. Putting this in a dedicated module ensures it is not forgotten or removed during refactoring.

## Motion Module Refactoring (Confidence: HIGH)

`ptz_motion` is solid and stays mostly as-is. Two additions needed:

1. **Per-axis velocity setters:** The current `setVelocity(pan, tilt, zoom)` sets all three at once. OSC per-axis commands need per-axis setters to avoid zeroing other axes.

```cpp
// Add to PtzMotion:
void setPanVelocity(float panNorm);
void setTiltVelocity(float tiltNorm);
void setZoomVelocity(float zoomNorm);
```

2. **Speed preset application:** A method to apply a `SpeedPreset` struct to all three axes at runtime.

```cpp
void applySpeedPreset(const SpeedPreset& preset);
```

The existing `setVelocity(pan, tilt, zoom)` stays for the combined `/ptz/move` command.

## Revised Main Loop

```cpp
// Simplified main.cpp after overhaul

#include "ptz_config.h"
#include "ptz_log.h"
#include "ptz_mdns.h"
#include "ptz_motion.h"
#include "ptz_osc.h"
#include "ptz_wifi.h"

namespace {
    ptz::PtzMotion g_motion;
    ptz::PtzOsc g_osc;
    ptz::PtzWifi g_wifi;
    ptz::PtzMdns g_mdns;

    uint32_t g_lastMicros = 0;
    uint32_t g_lastStatusMs = 0;
    uint32_t g_idleStartMs = 0;
    uint8_t g_speedPreset = ptz::kDefaultPresetIndex;
}

void setup() {
    Serial.begin(115200);
    ptz::logInit();

    g_motion.begin();
    g_wifi.begin(false);
    g_mdns.begin("ptzhead", ptz::kOscPort);
    g_osc.begin(&g_motion, ptz::kOscPort);
}

void loop() {
    const uint32_t nowMs = millis();
    float dt = computeDt();

    g_osc.update();
    handleSerialCommands();
    g_motion.update(dt);
    g_motion.run();

    // Idle shutdown (same logic, simplified without owner check)
    if (!g_motion.isMoving()) {
        // ... idle timeout logic
    }

    // Status broadcast
    if (nowMs - g_lastStatusMs >= ptz::kStatusIntervalMs) {
        g_osc.sendStatus(g_motion.state(), g_motion.isMoving(),
                         g_speedPreset, WiFi.RSSI());
        g_lastStatusMs = nowMs;
    }
}
```

**What disappeared:** The entire ownership system (g_owner, Owner enum, gamepad control checks, owner change detection). The gamepad update and commands struct. The preset save/recall system (out of scope). About 60 lines of orchestration logic in loop() vanishes.

## Suggested Build Order

Build order follows dependency chains. Each step produces testable firmware.

```
Step 1: Platform switch + cleanup
  - Switch platformio.ini from Bluepad32 to standard espressif32
  - Remove ptz_gamepad, ptz_ws, ptz_owner source files
  - Remove Bluepad32, WebSockets, ArduinoJson from lib_deps
  - Add CNMAT OSC library
  - Simplify main.cpp to just motion (no input)
  - TEST: Firmware compiles and boots, motors idle

Step 2: OSC input (receive)
  - Create ptz_osc module with WiFiUDP listener
  - Implement /ptz/pan, /ptz/tilt, /ptz/zoom, /ptz/move, /ptz/stop
  - Wire into main loop
  - TEST: Send OSC from desktop tool (oscsend/Protokol), motors respond

Step 3: OSC output (status feedback)
  - Add reply-to-sender IP caching
  - Implement sendStatus() with OSCBundle
  - Wire status broadcast into main loop
  - TEST: Receive status in desktop OSC monitor

Step 4: mDNS
  - Create ptz_mdns module
  - Advertise hostname + OSC service
  - TEST: dns-sd -B _osc._udp discovers the device

Step 5: Speed presets
  - Add SpeedPreset struct to config
  - Add /ptz/speed handler
  - Add per-axis velocity setters to motion
  - TEST: Switch presets via OSC, verify speed changes

Step 6: Companion integration
  - Configure Companion generic-osc module
  - Map StreamDeck buttons to OSC commands
  - Verify hold-to-move, release-to-stop behavior
  - TEST: End-to-end StreamDeck -> motor movement
```

**Why this order:**
- Step 1 must come first: unblocks everything by removing Bluepad32 platform dependency.
- Step 2 before Step 3: you need input before output matters.
- Step 4 (mDNS) is independent, slotted here because it is small and improves Step 6 DX.
- Step 5 is additive, not structural -- can come late.
- Step 6 is integration testing, not firmware development.

## Scalability Considerations

| Concern | Current (1 controller) | At 3-5 controllers | At 10+ controllers |
|---------|----------------------|--------------------|--------------------|
| Network | Single UDP socket, trivial | Each head has own IP/port, Companion handles routing | Same, Companion scales linearly |
| Discovery | mDNS sufficient | mDNS sufficient, each head unique hostname | mDNS still works, but consider a discovery page |
| Latency | Sub-1ms parse + dispatch | No change (independent devices) | No change |
| Configuration | Constants in firmware | Consider EEPROM/NVS for per-device tuning | NVS + a config OSC namespace |

Scalability is not a concern for this project. Each PTZ head is an independent device. Adding more heads means more Companion connections, not more firmware complexity.

## Sources

- [CNMAT OSC Library (GitHub)](https://github.com/CNMAT/OSC) -- v3.5.8, ESP32 supported
- [Bitfocus Companion Generic OSC Module](https://github.com/bitfocus/companion-module-generic-osc/blob/master/companion/HELP.md) -- bidirectional OSC over UDP
- [ESP32 mDNS Service Advertisement](https://techtutorialsx.com/2020/04/18/esp32-advertise-service-with-mdns/) -- ESPmDNS built into Arduino core
- [Arduino-ESP32 ESPmDNS Examples](https://github.com/espressif/arduino-esp32/blob/master/libraries/ESPmDNS/examples/mDNS_Web_Server/mDNS_Web_Server.ino) -- official examples
- Existing codebase: `src/main.cpp`, `src/ptz_motion.cpp`, `src/ptz_ws.h` -- current architecture patterns
