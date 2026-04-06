# Companion StreamDeck Button Configuration Guide

**Date:** 2026-04-06
**Target:** Bitfocus Companion with generic-osc module
**StreamDeck:** 5 columns x 3 rows (15 buttons)
**Firmware:** PTZHead v2

## Connection Setup

1. Open Companion web UI (default: `http://localhost:8000`)
2. Go to **Connections** tab
3. Add a new connection: **generic-osc**
4. Configure:
   - **Label:** `PTZHead` (or any name you prefer)
   - **Target IP:** `<ESP32 IP from serial log>` (e.g., `192.168.1.xxx`)
   - **Alternative:** Use `ptzhead.local` if mDNS confirmed working (Test Checklist Stage 6)
   - **Target port:** `8000`
   - **Protocol:** UDP
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

This D-pad layout places pan/tilt controls in a natural directional arrangement (center-left), zoom on the right side, speed presets surrounding the D-pad, and status displays in the rightmost column.

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
| **Button style** | Text: "SLOW", default bg: dark, active bg: green |

---

### Button 2: Tilt Up (Row 1, Col 2)

| Property | Value |
|----------|-------|
| **Label** | `^` (up arrow) |
| **Press action** | generic-osc > Send float |
| **Path** | `/ptz/tilt` |
| **Value** | `1.0` |
| **Repeat interval** | `100` ms |
| **Release action** | generic-osc > Send float |
| **Release path** | `/ptz/tilt` |
| **Release value** | `0.0` |
| **Feedback** | generic-osc > OSC listener |
| **Feedback path** | `/ptz/status/tilt/moving` |
| **Feedback rule** | When value = `1`: background green |
| **Button style** | Text: "^", default bg: dark |

> **CRITICAL:** The repeat interval of 100ms (10Hz) is essential for heartbeat watchdog compatibility. Without it, the motor stops after 500ms because the firmware receives no new OSC command and the heartbeat timeout fires.

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
| **Button style** | Text: "FAST", default bg: dark, active bg: green |

---

### Button 4: Zoom In (Row 1, Col 4)

| Property | Value |
|----------|-------|
| **Label** | `Z+` |
| **Press action** | generic-osc > Send float |
| **Path** | `/ptz/zoom` |
| **Value** | `1.0` |
| **Repeat interval** | `100` ms |
| **Release action** | generic-osc > Send float |
| **Release path** | `/ptz/zoom` |
| **Release value** | `0.0` |
| **Feedback** | generic-osc > OSC listener |
| **Feedback path** | `/ptz/status/zoom/moving` |
| **Feedback rule** | When value = `1`: background green |
| **Button style** | Text: "Z+", default bg: dark |

---

### Button 5: RSSI Display (Row 1, Col 5)

| Property | Value |
|----------|-------|
| **Label** | `RSSI` |
| **Press action** | None (display-only) |
| **Release action** | None |
| **Feedback** | generic-osc > OSC listener |
| **Feedback path** | `/ptz/status/rssi` |
| **Button text** | `RSSI\n$(generic-osc:PTZHead:latest_received_args)` |
| **Button style** | Text: "RSSI", default bg: dark |

> Replace `PTZHead` with whatever connection label you used in the connection setup. The RSSI value updates every ~1 second and shows a negative dBm integer (e.g., -55).

---

### Button 6: Pan Left (Row 2, Col 1)

| Property | Value |
|----------|-------|
| **Label** | `<` (left arrow) |
| **Press action** | generic-osc > Send float |
| **Path** | `/ptz/pan` |
| **Value** | `-1.0` |
| **Repeat interval** | `100` ms |
| **Release action** | generic-osc > Send float |
| **Release path** | `/ptz/pan` |
| **Release value** | `0.0` |
| **Feedback** | generic-osc > OSC listener |
| **Feedback path** | `/ptz/status/pan/moving` |
| **Feedback rule** | When value = `1`: background green |
| **Button style** | Text: "<", default bg: dark |

---

### Button 7: STOP ALL (Row 2, Col 2)

| Property | Value |
|----------|-------|
| **Label** | `STOP` |
| **Press action** | generic-osc > Send message (no arguments) |
| **Path** | `/ptz/stop` |
| **Release action** | None |
| **Feedback** | None |
| **Button style** | Text: "STOP", background: red (always) |

---

### Button 8: Pan Right (Row 2, Col 3)

| Property | Value |
|----------|-------|
| **Label** | `>` (right arrow) |
| **Press action** | generic-osc > Send float |
| **Path** | `/ptz/pan` |
| **Value** | `1.0` |
| **Repeat interval** | `100` ms |
| **Release action** | generic-osc > Send float |
| **Release path** | `/ptz/pan` |
| **Release value** | `0.0` |
| **Feedback** | generic-osc > OSC listener |
| **Feedback path** | `/ptz/status/pan/moving` |
| **Feedback rule** | When value = `1`: background green |
| **Button style** | Text: ">", default bg: dark |

---

### Button 9: Zoom Out (Row 2, Col 4)

| Property | Value |
|----------|-------|
| **Label** | `Z-` |
| **Press action** | generic-osc > Send float |
| **Path** | `/ptz/zoom` |
| **Value** | `-1.0` |
| **Repeat interval** | `100` ms |
| **Release action** | generic-osc > Send float |
| **Release path** | `/ptz/zoom` |
| **Release value** | `0.0` |
| **Feedback** | generic-osc > OSC listener |
| **Feedback path** | `/ptz/status/zoom/moving` |
| **Feedback rule** | When value = `1`: background green |
| **Button style** | Text: "Z-", default bg: dark |

---

### Button 10: Preset Display (Row 2, Col 5)

| Property | Value |
|----------|-------|
| **Label** | `SPD` |
| **Press action** | None (display-only) |
| **Release action** | None |
| **Feedback** | generic-osc > OSC listener |
| **Feedback path** | `/ptz/status/preset` |
| **Button text** | `SPD\n$(generic-osc:PTZHead:latest_received_args)` |
| **Button style** | Text: "SPD", default bg: dark |

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
| **Button style** | Text: "MED", default bg: dark, active bg: green |

---

### Button 12: Tilt Down (Row 3, Col 2)

| Property | Value |
|----------|-------|
| **Label** | `v` (down arrow) |
| **Press action** | generic-osc > Send float |
| **Path** | `/ptz/tilt` |
| **Value** | `-1.0` |
| **Repeat interval** | `100` ms |
| **Release action** | generic-osc > Send float |
| **Release path** | `/ptz/tilt` |
| **Release value** | `0.0` |
| **Feedback** | generic-osc > OSC listener |
| **Feedback path** | `/ptz/status/tilt/moving` |
| **Feedback rule** | When value = `1`: background green |
| **Button style** | Text: "v", default bg: dark |

---

### Buttons 13-15: Empty (Row 3, Col 3-5)

No configuration needed. Leave blank or add custom labels/functions as desired.

---

## Important Notes

### Repeat Interval is Critical

All motion buttons (Pan Left, Pan Right, Tilt Up, Tilt Down, Zoom In, Zoom Out) **MUST** have their press action repeat interval set to `100` ms (10Hz). Without this, the firmware's 500ms heartbeat watchdog will auto-stop the motor after a single press because no new OSC commands arrive.

**Symptom if missing:** Motor starts then stops after ~500ms despite button being held.

### Press + Release Pattern

All motion buttons use a press-and-release pattern:
- **Press** sends the velocity value (e.g., `1.0`) and repeats at 100ms
- **Release** sends `0.0` as a redundant stop safety

The release action is a safety net. The heartbeat watchdog is the primary stop mechanism, but sending `0.0` on release ensures immediate stop without waiting for the 500ms timeout.

### Speed Preset Buttons Do NOT Need Repeat

Speed preset buttons (Slow, Medium, Fast) send a single integer on press. No repeat interval is needed -- a preset change is a one-shot command that persists until another preset is selected.

### Direction Fix

If a motor moves in the wrong direction (tested in Test Checklist Stage 2), you have two options:
1. **Firmware fix:** Flip `kInvertPan` (or add equivalent for tilt/zoom) in `src/ptz_config.h`
2. **Companion fix:** Swap the positive/negative values for that axis's buttons (e.g., Pan Left sends `1.0` instead of `-1.0`)

### Backup Your Configuration

After configuring all buttons, use Companion's **Export** feature to save the configuration as a `.companionconfig` file. This allows you to restore the layout on a new Companion installation without reconfiguring each button.

### Connection Target: IP vs mDNS

- Use the ESP32's IP address (from serial log) for initial setup
- After confirming mDNS works (Test Checklist Stage 6), you can switch to `ptzhead.local` as the target
- mDNS is more convenient (survives DHCP lease changes) but requires mDNS support on the Companion host

## Quick Reference Table

| # | Position | Label | OSC Path | Type | Value | Repeat | Release |
|---|----------|-------|----------|------|-------|--------|---------|
| 1 | R1C1 | SLOW | `/ptz/speed/preset` | int | `0` | -- | -- |
| 2 | R1C2 | ^ | `/ptz/tilt` | float | `1.0` | 100ms | `0.0` |
| 3 | R1C3 | FAST | `/ptz/speed/preset` | int | `2` | -- | -- |
| 4 | R1C4 | Z+ | `/ptz/zoom` | float | `1.0` | 100ms | `0.0` |
| 5 | R1C5 | RSSI | `/ptz/status/rssi` | display | -- | -- | -- |
| 6 | R2C1 | < | `/ptz/pan` | float | `-1.0` | 100ms | `0.0` |
| 7 | R2C2 | STOP | `/ptz/stop` | none | -- | -- | -- |
| 8 | R2C3 | > | `/ptz/pan` | float | `1.0` | 100ms | `0.0` |
| 9 | R2C4 | Z- | `/ptz/zoom` | float | `-1.0` | 100ms | `0.0` |
| 10 | R2C5 | SPD | `/ptz/status/preset` | display | -- | -- | -- |
| 11 | R3C1 | MED | `/ptz/speed/preset` | int | `1` | -- | -- |
| 12 | R3C2 | v | `/ptz/tilt` | float | `-1.0` | 100ms | `0.0` |
| 13 | R3C3 | -- | -- | -- | -- | -- | -- |
| 14 | R3C4 | -- | -- | -- | -- | -- | -- |
| 15 | R3C5 | -- | -- | -- | -- | -- | -- |
