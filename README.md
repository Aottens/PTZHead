# PTZHead Firmware

PlatformIO firmware for an ESP32-based PTZ head with stepper control, Bluepad32 gamepad input, WiFi provisioning, and a WebSocket JSON API.

## Build & Upload

```sh
pio run
pio run -t upload
```

## Serial Monitor

```sh
pio device monitor
```

## WiFi Provisioning

* On boot, the firmware attempts stored credentials and falls back to the captive portal SSID `PTZHead Setup`.
* Send `WIFI RESET` over serial or hold the gamepad combo **L1 + R1 + X + Y** for 2 seconds to reset credentials and reopen the portal.
