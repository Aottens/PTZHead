# Companion StreamDeck Button Configuration Guide

**Date:** 2026-04-09
**Target:** Bitfocus Companion v4.2.6 with generic-osc module v2.8.2
**StreamDeck:** 5 columns x 3 rows (15 buttons)
**Firmware:** PTZHead v2

## Connection Setup

1. Open Companion web UI (default: `http://localhost:8000`)
2. Go to **Connections** tab
3. Add a new connection: **Generic: OSC**
4. Configure:
   - **Label:** `PTZ-Head` (or any name you prefer)
   - **Target Hostname or IP:** `192.168.50.74` (check serial log for current IP)
   - **Alternative:** Use `ptzhead.local` if mDNS confirmed working (Test Checklist Stage 6)
   - **Target Port:** `8000`
   - **Protocol:** UDP (Default)
   - **Listen for Feedback:** Enable this for feedback buttons (RSSI, preset, moving state)
5. Save the connection

## Layout Diagram

```
         Col 1        Col 2        Col 3        Col 4        Col 5
       +-----------+-----------+-----------+-----------+-----------+
Row 1  |   SLOW    |  TILT UP  |   FAST    |  ZOOM IN  |   RSSI   |
       |  preset 0 |     ^     |  preset 2 |    Z+     |  display  |
       +-----------+-----------+-----------+-----------+-----------+
Row 2  | PAN LEFT  | STOP ALL  | PAN RIGHT | ZOOM OUT  |  PRESET  |
       |     <     |   STOP    |     >     |    Z-     |  display  |
       +-----------+-----------+-----------+-----------+-----------+
Row 3  |  MEDIUM   | TILT DOWN |   ---     |   ---     |   ---    |
       |  preset 1 |     v     |           |           |           |
       +-----------+-----------+-----------+-----------+-----------+
```

D-pad layout: pan/tilt in natural directional arrangement (center-left), zoom on the right, speed presets surrounding the D-pad, status displays in the rightmost column.

---

## How Motion Buttons Work

All motion buttons use a simple **press/release** pattern:
- **Press** sends the velocity value (e.g., `1.0`) — motor starts
- **Release** sends `0.0` — motor stops

No repeat or loop is needed. The firmware has a 5-second heartbeat timeout as a safety net — if the release message is lost (WiFi drop, Companion crash), the motor auto-stops after 5 seconds.

---

## Button-by-Button Configuration

### Button 1: Slow (Row 1, Col 1)

| Property | Value |
|----------|-------|
| **Label** | `SLOW` |
| **Press action** | generic-osc > Send integer |
| **Path** | `/ptz/speed/preset` |
| **Value** | `0` |
| **Release action** | None |
| **Feedback** | generic-osc > OSC listener |
| **Feedback path** | `/ptz/status/preset` |
| **Feedback rule** | When value = `0`: background green; otherwise: default |

---

### Button 2: Tilt Up (Row 1, Col 2)

| Property | Value |
|----------|-------|
| **Label** | `^` (up arrow) |
| **Press action** | generic-osc > Send float |
| **Path** | `/ptz/tilt` |
| **Value** | `1.0` |
| **Release action** | generic-osc > Send float |
| **Release path** | `/ptz/tilt` |
| **Release value** | `0.0` |
| **Feedback** | generic-osc > OSC listener |
| **Feedback path** | `/ptz/status/tilt/moving` |
| **Feedback rule** | When value = `1`: background green |

---

### Button 3: Fast (Row 1, Col 3)

| Property | Value |
|----------|-------|
| **Label** | `FAST` |
| **Press action** | generic-osc > Send integer |
| **Path** | `/ptz/speed/preset` |
| **Value** | `2` |
| **Release action** | None |
| **Feedback** | generic-osc > OSC listener |
| **Feedback path** | `/ptz/status/preset` |
| **Feedback rule** | When value = `2`: background green; otherwise: default |

---

### Button 4: Zoom In (Row 1, Col 4)

| Property | Value |
|----------|-------|
| **Label** | `Z+` |
| **Press action** | generic-osc > Send float |
| **Path** | `/ptz/zoom` |
| **Value** | `1.0` |
| **Release action** | generic-osc > Send float |
| **Release path** | `/ptz/zoom` |
| **Release value** | `0.0` |
| **Feedback** | generic-osc > OSC listener |
| **Feedback path** | `/ptz/status/zoom/moving` |
| **Feedback rule** | When value = `1`: background green |

---

### Button 5: RSSI Display (Row 1, Col 5)

| Property | Value |
|----------|-------|
| **Label** | `RSSI` |
| **Press action** | None (display-only) |
| **Release action** | None |
| **Feedback** | generic-osc > OSC listener |
| **Feedback path** | `/ptz/status/rssi` |
| **Button text** | `RSSI\n$(generic-osc:PTZ-Head:latest_received_args)` |

> Replace `PTZ-Head` with your connection label. Shows a negative dBm integer (e.g., -55), updated every ~1 second.

---

### Button 6: Pan Left (Row 2, Col 1)

| Property | Value |
|----------|-------|
| **Label** | `<` (left arrow) |
| **Press action** | generic-osc > Send float |
| **Path** | `/ptz/pan` |
| **Value** | `-1.0` |
| **Release action** | generic-osc > Send float |
| **Release path** | `/ptz/pan` |
| **Release value** | `0.0` |
| **Feedback** | generic-osc > OSC listener |
| **Feedback path** | `/ptz/status/pan/moving` |
| **Feedback rule** | When value = `1`: background green |

---

### Button 7: STOP ALL (Row 2, Col 2)

| Property | Value |
|----------|-------|
| **Label** | `STOP` |
| **Press action** | generic-osc > Send message (no arguments) |
| **Path** | `/ptz/stop` |
| **Release action** | None |
| **Button style** | Text: "STOP", background: red (always) |

---

### Button 8: Pan Right (Row 2, Col 3)

| Property | Value |
|----------|-------|
| **Label** | `>` (right arrow) |
| **Press action** | generic-osc > Send float |
| **Path** | `/ptz/pan` |
| **Value** | `1.0` |
| **Release action** | generic-osc > Send float |
| **Release path** | `/ptz/pan` |
| **Release value** | `0.0` |
| **Feedback** | generic-osc > OSC listener |
| **Feedback path** | `/ptz/status/pan/moving` |
| **Feedback rule** | When value = `1`: background green |

---

### Button 9: Zoom Out (Row 2, Col 4)

| Property | Value |
|----------|-------|
| **Label** | `Z-` |
| **Press action** | generic-osc > Send float |
| **Path** | `/ptz/zoom` |
| **Value** | `-1.0` |
| **Release action** | generic-osc > Send float |
| **Release path** | `/ptz/zoom` |
| **Release value** | `0.0` |
| **Feedback** | generic-osc > OSC listener |
| **Feedback path** | `/ptz/status/zoom/moving` |
| **Feedback rule** | When value = `1`: background green |

---

### Button 10: Preset Display (Row 2, Col 5)

| Property | Value |
|----------|-------|
| **Label** | `SPD` |
| **Press action** | None (display-only) |
| **Release action** | None |
| **Feedback** | generic-osc > OSC listener |
| **Feedback path** | `/ptz/status/preset` |
| **Button text** | `SPD\n$(generic-osc:PTZ-Head:latest_received_args)` |

> Shows the current speed preset index: 0 = Slow, 1 = Medium, 2 = Fast.

---

### Button 11: Medium (Row 3, Col 1)

| Property | Value |
|----------|-------|
| **Label** | `MED` |
| **Press action** | generic-osc > Send integer |
| **Path** | `/ptz/speed/preset` |
| **Value** | `1` |
| **Release action** | None |
| **Feedback** | generic-osc > OSC listener |
| **Feedback path** | `/ptz/status/preset` |
| **Feedback rule** | When value = `1`: background green; otherwise: default |

---

### Button 12: Tilt Down (Row 3, Col 2)

| Property | Value |
|----------|-------|
| **Label** | `v` (down arrow) |
| **Press action** | generic-osc > Send float |
| **Path** | `/ptz/tilt` |
| **Value** | `-1.0` |
| **Release action** | generic-osc > Send float |
| **Release path** | `/ptz/tilt` |
| **Release value** | `0.0` |
| **Feedback** | generic-osc > OSC listener |
| **Feedback path** | `/ptz/status/tilt/moving` |
| **Feedback rule** | When value = `1`: background green |

---

### Buttons 13-15: Empty (Row 3, Col 3-5)

No configuration needed. Leave blank or add custom labels/functions as desired.

---

## Important Notes

### Direction Fix

If a motor moves in the wrong direction:
1. **Firmware fix:** Flip `kInvertPan` (or add equivalent for tilt/zoom) in `src/ptz_config.h`
2. **Companion fix:** Swap the positive/negative values for that axis's buttons

### Safety: Heartbeat Timeout

The firmware has a 5-second heartbeat timeout. If no OSC packet arrives within 5 seconds of the last one, all motors auto-stop. This is a safety net for:
- WiFi dropout while a motor is running
- Companion crash while holding a button
- Any other scenario where the release message never arrives

### Backup Your Configuration

After configuring all buttons, use Companion's **Export** feature to save as a `.companionconfig` file for backup.

### Connection Target: IP vs mDNS

- Use the ESP32's IP address (from serial log) for initial setup
- After confirming mDNS works (Test Checklist Stage 6), switch to `ptzhead.local`
- mDNS survives DHCP lease changes but requires mDNS support on the Companion host

## Quick Reference Table

| # | Position | Label | OSC Path | Type | Press Value | Release Value |
|---|----------|-------|----------|------|-------------|---------------|
| 1 | R1C1 | SLOW | `/ptz/speed/preset` | int | `0` | -- |
| 2 | R1C2 | ^ | `/ptz/tilt` | float | `1.0` | `0.0` |
| 3 | R1C3 | FAST | `/ptz/speed/preset` | int | `2` | -- |
| 4 | R1C4 | Z+ | `/ptz/zoom` | float | `1.0` | `0.0` |
| 5 | R1C5 | RSSI | `/ptz/status/rssi` | display | -- | -- |
| 6 | R2C1 | < | `/ptz/pan` | float | `-1.0` | `0.0` |
| 7 | R2C2 | STOP | `/ptz/stop` | none | -- | -- |
| 8 | R2C3 | > | `/ptz/pan` | float | `1.0` | `0.0` |
| 9 | R2C4 | Z- | `/ptz/zoom` | float | `-1.0` | `0.0` |
| 10 | R2C5 | SPD | `/ptz/status/preset` | display | -- | -- |
| 11 | R3C1 | MED | `/ptz/speed/preset` | int | `1` | -- |
| 12 | R3C2 | v | `/ptz/tilt` | float | `-1.0` | `0.0` |
| 13-15 | R3C3-5 | -- | -- | -- | -- | -- |
