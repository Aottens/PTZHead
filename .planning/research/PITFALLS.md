# Domain Pitfalls

**Domain:** ESP32 OSC-controlled PTZ camera head (stepper motors over WiFi/UDP)
**Researched:** 2026-04-03

## Critical Pitfalls

Mistakes that cause rewrites, missed steps, or unreliable motion.

### Pitfall 1: WiFi Power Save Causes UDP Latency Spikes (100-300ms)

**What goes wrong:** The current code in `ptz_wifi.cpp` enables modem sleep with `esp_wifi_set_ps(WIFI_PS_MIN_MODEM)`. In this mode, the ESP32 radio sleeps between DTIM beacon intervals (typically 100-300ms depending on router settings). Incoming UDP packets queue at the access point until the next beacon, causing unpredictable latency spikes. For a hold-to-move PTZ interface where the PROJECT.md requires sub-50ms response, this is a direct violation of the latency constraint.

**Why it happens:** Power save is a reasonable default for battery devices, and the existing code was designed for a gamepad (Bluetooth, no WiFi latency concern for input). When switching to OSC-over-WiFi as the primary control path, power save becomes the enemy.

**Consequences:** Button press on StreamDeck feels sluggish or inconsistent. Stop commands arrive late, causing overshoot. Motion feels "spongy" with variable response times that undermine operator confidence.

**Prevention:**
- Call `esp_wifi_set_ps(WIFI_PS_NONE)` immediately after WiFi connects. This is a one-line fix but critical.
- Also call `WiFi.setSleep(false)` for belt-and-suspenders (the Arduino wrapper).
- Power consumption increases (~80mA more), but the device is mains-powered so this is irrelevant.

**Detection:** Measure round-trip time: send an OSC ping from Companion, have ESP32 reply immediately, log the delta. If you see >20ms spikes, power save is likely still active.

**Phase:** Must be addressed in the very first WiFi/OSC integration phase. Non-negotiable.

### Pitfall 2: AccelStepper run() Starved by Network Processing

**What goes wrong:** AccelStepper generates step pulses in software -- each call to `run()` checks timing and toggles a GPIO pin. If `run()` is not called frequently enough (every ~250us at 4000 steps/sec), the motor misses steps, runs slower than commanded, or stutters. The current `loop()` calls `run()` every iteration, which works fine without network overhead. Adding UDP parsing (packet receive, OSC decode, string matching) in the same loop adds microseconds-to-milliseconds of jitter per iteration. WiFi stack background tasks on core 0 can also preempt or delay core 1 tasks via shared resource contention.

**Why it happens:** AccelStepper is a polling library. It has zero hardware timer backing. Every microsecond spent doing non-motor work is a microsecond where a step pulse could be missed. The existing code at `kPanMaxSps = 4000.0f` needs `run()` called at minimum every 250us for full-speed operation.

**Consequences:** Motors run slower than commanded. Audible stuttering. Pan/tilt axes lose synchronization (one axis gets more CPU time than another). At worst, missed steps cause position drift.

**Prevention:** Two options, in order of preference:

1. **Switch to FastAccelStepper** (recommended): Uses ESP32 hardware peripherals (RMT or MCPWM/PCNT) to generate step pulses autonomously. Once you set a speed, the hardware generates pulses without any `run()` polling. This completely decouples motor timing from loop jitter. The API is similar to AccelStepper. This is the single highest-impact architectural change for reliability.

2. **Pin motor task to core 1, network to core 0**: Use `xTaskCreatePinnedToCore` to run a dedicated motor loop on core 1 at high priority, and handle OSC/WiFi on core 0. This preserves AccelStepper but adds FreeRTOS complexity and still leaves software stepping vulnerable to interrupt jitter.

**Detection:** Log `loop()` iteration time. If any iteration exceeds 250us, you are at risk. Add `micros()` instrumentation around the OSC receive/parse section specifically.

**Phase:** Evaluate during initial architecture phase. If sticking with AccelStepper, implement dual-core in the motor integration phase. If switching to FastAccelStepper, do it early since it changes the motion API.

### Pitfall 3: UDP Packet Loss with No Application-Level Recovery

**What goes wrong:** UDP is fire-and-forget. OSC over UDP inherits this -- there is no ACK, no retry, no sequence numbering. On WiFi (not Ethernet), packet loss rates of 1-5% are normal under good conditions, higher with interference or weak signal. A lost "stop" command means the head keeps moving. A lost "start" command means nothing happens on button press. Both are bad for a live camera operator.

**Why it happens:** OSC was designed for local networks (often wired) in music/performance contexts where occasional dropped messages are tolerable. A PTZ head has safety implications -- runaway motion can damage cables or hit limits.

**Consequences:** Head continues moving after operator releases button (lost stop). Operator presses button and nothing happens (lost start). In production, this erodes trust and can cause physical damage.

**Prevention:**
- **Heartbeat/watchdog pattern**: Companion sends periodic heartbeat (e.g., every 200ms). ESP32 implements a watchdog -- if no heartbeat received for 500-750ms, stop all axes. This is the single most important safety feature.
- **Redundant stop commands**: On button release, Companion should send 2-3 stop commands spaced 10-20ms apart (configurable in Companion button actions).
- **Continuous velocity mode**: Instead of discrete start/stop, have Companion send velocity at 10-20Hz while button is held. If messages stop arriving, the watchdog catches it. This is more robust than single start/stop events.
- Do NOT attempt to add TCP or reliability to OSC. It defeats the purpose and adds latency.

**Detection:** Log received OSC message rate. If it drops below expected heartbeat rate, trigger warning. Monitor the watchdog trigger count over time.

**Phase:** Heartbeat/watchdog must be in the core OSC implementation phase, not deferred as a "nice to have." Design the OSC command schema around continuous-send from the start.

### Pitfall 4: mDNS Stops Responding After Minutes

**What goes wrong:** ESP32 mDNS has a well-documented bug where it stops responding to queries after approximately 2 minutes. The service broadcasts an mDNS announcement with a 2-minute TTL at startup, and if `MDNS.addService()` is not called, mDNS goes silent after that TTL expires. Even with the service registered, WiFi sleep mode causes mDNS query responses to be missed.

**Why it happens:** The ESP32 mDNS implementation has had reliability issues across multiple Arduino core versions (documented in issues #4406, #1478, #7156 on the arduino-esp32 repo). WiFi sleep mode (even MIN_MODEM) causes the radio to miss incoming mDNS queries during sleep intervals.

**Consequences:** `ptzhead.local` works for the first couple of minutes after boot, then Companion can no longer resolve the address. Operator thinks the device is offline. Debugging is frustrating because rebooting "fixes" it temporarily.

**Prevention:**
- Always call `MDNS.addService("_osc", "_udp", port)` after `MDNS.begin("ptzhead")`.
- Disable WiFi sleep (`WIFI_PS_NONE`) -- this also fixes mDNS (see Pitfall 1).
- **Always provide a fixed IP fallback** in the Companion configuration. mDNS is convenient but not reliable enough to be the only discovery mechanism.
- Consider periodic mDNS re-announcement (call `MDNS.announce()` every 60 seconds if available in your core version).

**Detection:** From a laptop on the same network, run `ping ptzhead.local` continuously. If it stops resolving after 2-5 minutes, mDNS is broken.

**Phase:** Address in the network/discovery phase. Implement fixed IP fallback first, mDNS as convenience layer on top.

## Moderate Pitfalls

### Pitfall 5: Framework Switch Breaks Partition Table or Flash Layout

**What goes wrong:** The current `platformio.ini` uses a Bluepad32-patched Arduino core (`pio-framework-bluepad32`). Switching to the standard `espressif32` Arduino framework changes the underlying ESP-IDF version, potentially changing the default partition table, NVS layout, or flash offsets. WiFiManager stores credentials in NVS. If the NVS partition moves or the format changes, stored WiFi credentials are lost or corrupted, and the device falls into provisioning mode unexpectedly.

**Why it happens:** Bluepad32's custom framework pins a specific ESP-IDF version. The standard espressif32 platform at version 6.10.0 may use a different ESP-IDF base. Partition tables are not always forward-compatible.

**Prevention:**
- Before the switch, document the current partition table (`pio run -t partitions`).
- After switching to standard framework, explicitly set the same partition table in `platformio.ini` with `board_build.partitions`.
- Test WiFiManager credential persistence across the framework change.
- Have a "factory reset" path (the existing serial `WIFI RESET` command) documented for users.

**Detection:** After framework switch, if the device boots into provisioning portal despite previously working credentials, the NVS layout changed.

**Phase:** Address first, before any code changes. The framework switch is a prerequisite for everything else.

### Pitfall 6: OSC Message Parsing Allocates on Heap in Hot Loop

**What goes wrong:** Some OSC libraries (notably CNMAT/OSC) use dynamic memory allocation (`new`, `String`) internally when parsing messages. In a tight loop running at 1kHz+, heap fragmentation accumulates over hours/days, eventually causing allocation failures or watchdog resets. The ESP32 has ~320KB RAM; heap fragmentation on long-running embedded systems is a known failure mode.

**Why it happens:** OSC libraries designed for desktop or Teensy don't optimize for embedded heap constraints. CNMAT/OSC stores OSC data using `OSCData` objects allocated on the heap. MicroOsc avoids this by operating on raw buffers without internal allocation.

**Prevention:**
- **Use MicroOsc** (or similar zero-allocation library) instead of CNMAT/OSC. MicroOsc parses in-place on a caller-provided buffer with no internal allocation.
- If using CNMAT/OSC, pre-allocate a message pool and reuse objects. Avoid creating/destroying `OSCMessage` objects per packet.
- Monitor free heap over time with `ESP.getFreeHeap()` logging every 60 seconds. If it trends downward, you have a leak or fragmentation.

**Detection:** Log `ESP.getFreeHeap()` periodically. A steady decline over hours indicates fragmentation. A sudden crash after 12-48 hours of uptime is the classic symptom.

**Phase:** OSC library selection phase. Choose the right library upfront to avoid a mid-project rewrite.

### Pitfall 7: WiFi Disconnection Without Automatic Recovery

**What goes wrong:** ESP32 WiFi auto-reconnect (`WiFi.setAutoReconnect(true)`) is unreliable and has been reported broken across multiple Arduino core versions (issue #653). When the access point reboots, goes out of range briefly, or the channel changes, the ESP32 may not reconnect. The current code in `ptz_wifi.cpp` does not set auto-reconnect and has no reconnection logic -- it connects once at boot and assumes the connection persists.

**Why it happens:** The Arduino ESP32 WiFi driver's auto-reconnect does not handle all disconnection reasons (issue #7210). Some disconnect codes simply don't trigger a reconnect attempt.

**Consequences:** After a brief WiFi dropout (router reboot, channel switch, interference), the PTZ head becomes unresponsive until physically power-cycled. In a production environment, this is unacceptable.

**Prevention:**
- Register a WiFi event handler for `ARDUINO_EVENT_WIFI_STA_DISCONNECTED`.
- In the handler, call `WiFi.reconnect()` after a short delay (500ms).
- Implement exponential backoff (500ms, 1s, 2s, 4s) to avoid hammering a temporarily unavailable AP.
- After N failed reconnects (e.g., 10), fall back to provisioning portal or reboot.
- When disconnected, the motor watchdog (Pitfall 3) should stop all axes immediately.

**Detection:** Log WiFi events. If `DISCONNECTED` events appear without subsequent `GOT_IP` events, reconnection is broken.

**Phase:** WiFi refactoring phase. Must be solid before OSC reliability can be evaluated.

### Pitfall 8: UDP Receive Buffer Overflow Under Burst Traffic

**What goes wrong:** Companion sends OSC at whatever rate the button actions fire. If multiple buttons are pressed simultaneously or actions trigger rapidly, multiple UDP packets arrive in quick succession. The ESP32 `WiFiUDP` receive buffer is small by default. If the loop doesn't call `parsePacket()` fast enough, older packets are silently dropped by the network stack. Additionally, the ESP32 WiFi stack can "bunch" UDP packets, delivering them in bursts of 200ms worth of data at once.

**Why it happens:** WiFi power save batches incoming packets. Even with power save disabled, the WiFi driver may briefly queue packets. The default UDP receive buffer is small (around 1-4 packets).

**Prevention:**
- Increase the UDP receive buffer: `udp.begin(port)` followed by setting socket options if needed, or process packets in a tight drain loop (`while (udp.parsePacket()) { ... }`).
- Always drain all available packets per loop iteration, not just one.
- Process the most recent packet if multiple are queued (latest velocity command supersedes older ones).

**Detection:** If motor response feels "chunky" (moves in bursts rather than smoothly), packets are being batched or dropped.

**Phase:** OSC receive implementation phase. Design the receive loop correctly from the start.

## Minor Pitfalls

### Pitfall 9: OSC Address Pattern Design Locks You Into Bad Schema

**What goes wrong:** Designing OSC addresses like `/pan/speed 0.5` seems intuitive but prevents future extension. Later you want per-axis enable/disable, speed presets, position queries, and the flat namespace becomes unwieldy. Renaming OSC addresses is a breaking change for all Companion button configurations.

**Prevention:**
- Use a hierarchical namespace: `/ptz/velocity <pan> <tilt> <zoom>` (single message, three args) rather than per-axis messages.
- Reserve namespaces: `/ptz/motion/*`, `/ptz/config/*`, `/ptz/status/*`.
- Document the OSC schema as a contract. Version it if needed (`/v1/ptz/...`).
- Send all three velocity values in one message to ensure atomic updates.

**Phase:** OSC schema design phase (before implementation).

### Pitfall 10: Serial Logging in Hot Path Causes Timing Jitter

**What goes wrong:** The existing code has rate-limited logging (`logShouldEmit`), which is good. But adding `PTZ_LOGI` calls in the OSC receive path or motor update path during development creates timing jitter. Serial.print at 115200 baud takes ~87us per character. A 50-character log line takes ~4.3ms -- enough to miss 17 step pulses at 4000 steps/sec.

**Prevention:**
- Never log in the motor `run()` path.
- Rate-limit all OSC-related logging aggressively (max once per second).
- Consider increasing baud rate to 921600 for development.
- Use the existing `logShouldEmit` pattern for any new log points.

**Phase:** Ongoing discipline throughout all phases. Set baud rate in the framework switch phase.

### Pitfall 11: WiFiManager Captive Portal Blocks the Main Loop

**What goes wrong:** `WiFiManager::startConfigPortal()` is a blocking call. While the provisioning portal is active, the main loop does not run, motors cannot be controlled, and no OSC processing occurs. This is by design, but if WiFiManager is accidentally triggered (e.g., due to a brief WiFi dropout being misinterpreted as "no credentials"), the device becomes completely unresponsive.

**Prevention:**
- Keep the existing pattern: WiFiManager runs only at boot, never re-entered from the main loop.
- The serial `WIFI RESET` command is the only path to re-provisioning (already implemented).
- Do NOT auto-trigger provisioning on WiFi disconnect (use reconnection logic from Pitfall 7 instead).
- Consider a physical button or GPIO for forced provisioning rather than serial-only.

**Phase:** WiFi refactoring phase. Ensure reconnection logic (Pitfall 7) is separate from provisioning logic.

## Phase-Specific Warnings

| Phase Topic | Likely Pitfall | Mitigation |
|-------------|---------------|------------|
| Framework switch (Bluepad32 to standard) | NVS/partition incompatibility (Pitfall 5) | Compare partition tables before and after switch |
| WiFi refactoring | Power save latency (Pitfall 1), no auto-reconnect (Pitfall 7) | Disable power save, add event-based reconnection |
| OSC library integration | Heap allocation in hot path (Pitfall 6), buffer overflow (Pitfall 8) | Choose MicroOsc or similar zero-alloc lib, drain all packets per loop |
| OSC schema design | Bad namespace design (Pitfall 9) | Design schema before coding, use hierarchical addresses |
| Motor + network integration | AccelStepper starvation (Pitfall 2) | Evaluate FastAccelStepper, or use dual-core pinning |
| Safety/reliability | UDP packet loss (Pitfall 3), mDNS failure (Pitfall 4) | Heartbeat watchdog, fixed IP fallback |
| Long-running stability | Heap fragmentation (Pitfall 6), WiFi dropout (Pitfall 7) | Monitor free heap, robust reconnection |

## Sources

- [ESP32 Modem Sleep high latency and loss rates - esp-idf#9766](https://github.com/espressif/esp-idf/issues/9766)
- [Tuning latency of WiFiUdp - arduino-esp32#1283](https://github.com/espressif/arduino-esp32/issues/1283)
- [Task Watchdog and AccelStepper - arduino-esp32#2892](https://github.com/espressif/arduino-esp32/issues/2892)
- [AccelStepper + WiFi problem - Arduino Forum](https://forum.arduino.cc/t/accelstepper-wifi-problem/978221)
- [WiFi task interfering with loop() on app core - ESP32 Forum](https://esp32.com/viewtopic.php?t=12325)
- [FastAccelStepper - hardware-backed stepper library](https://github.com/gin66/FastAccelStepper)
- [mDNS stops working after two minutes - arduino-esp32#4406](https://github.com/espressif/arduino-esp32/issues/4406)
- [mDNS unreliable unless WiFi.setSleep(false) - arduino-esp32#7156](https://github.com/espressif/arduino-esp32/issues/7156)
- [WiFi Auto Reconnect not working - arduino-esp32#653](https://github.com/espressif/arduino-esp32/issues/653)
- [WiFi auto reconnect for all disconnect reasons - arduino-esp32#7210](https://github.com/espressif/arduino-esp32/issues/7210)
- [ESP32 WiFi UDP bunching packets - Arduino Forum](https://forum.arduino.cc/t/esp32-wifi-udp-bunching-packets/1162055)
- [ESP32 Wi-Fi Power Save documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/wifi-driver/wifi-performance-and-power-save.html)
- [Smooth Stepper Motor Control with ESP32 Dual Core](https://protonestiot.medium.com/smooth-stepper-motor-control-with-esp32-using-5aa3a083c11c)
- [Marlin PR: Pin I2S stepper task to core 1 for WiFi compatibility](https://github.com/MarlinFirmware/Marlin/pull/16874/files)
- [Bitfocus Companion Generic OSC module](https://github.com/bitfocus/companion-module-generic-osc)
- [MicroOsc - lightweight OSC for Arduino](https://github.com/thomasfredericks/MicroOsc)
- [CNMAT/OSC - Arduino OSC library](https://github.com/CNMAT/OSC)
