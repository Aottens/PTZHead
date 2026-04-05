#include <Arduino.h>
#include <WiFi.h>

#include "ptz_config.h"
#include "ptz_log.h"
#include "ptz_motion.h"
#include "ptz_wifi.h"
#include "ptz_osc.h"

namespace {

ptz::PtzMotion g_motion;
ptz::PtzWifi g_wifi;
ptz::PtzOsc g_osc;

void handleSerialCommands() {
  static String line;
  while (Serial.available()) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r') {
      line.trim();
      if (line.length() == 0) { line = ""; continue; }

      if (line.equalsIgnoreCase("STOP")) {
        g_motion.stop();
        PTZ_LOGI("CMD", "All axes stopped");
      } else if (line.equalsIgnoreCase("WIFI RESET")) {
        PTZ_LOGI("CMD", "WiFi reset requested");
        g_wifi.resetAndProvision();
      } else if (line.startsWith("PAN ") || line.startsWith("pan ")) {
        float val = line.substring(4).toFloat();
        g_motion.setVelocity(val, 0.0f, 0.0f);
        PTZ_LOGI("CMD", "Pan velocity: %.2f", val);
      } else if (line.startsWith("TILT ") || line.startsWith("tilt ")) {
        float val = line.substring(5).toFloat();
        g_motion.setVelocity(0.0f, val, 0.0f);
        PTZ_LOGI("CMD", "Tilt velocity: %.2f", val);
      } else if (line.startsWith("ZOOM ") || line.startsWith("zoom ")) {
        float val = line.substring(5).toFloat();
        g_motion.setVelocity(0.0f, 0.0f, val);
        PTZ_LOGI("CMD", "Zoom velocity: %.2f", val);
      } else {
        PTZ_LOGW("CMD", "Unknown: %s", line.c_str());
      }
      line = "";
    } else {
      line += c;
    }
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);
  ptz::logInit();

  PTZ_LOGI("BOOT", "PTZHead starting");

  g_motion.begin();
  g_wifi.begin(false);
  g_osc.begin(&g_motion);

  PTZ_LOGI("BOOT", "Setup complete — OSC on UDP %u; serial: PAN/TILT/ZOOM, STOP, WIFI RESET", ptz::kOscPort);
}

void loop() {
  g_osc.update();
  g_osc.updateFeedback();
  handleSerialCommands();

  const uint32_t now = millis();
  if (g_motion.isMoving() && (now - g_osc.lastRxMs()) > ptz::kHeartbeatTimeoutMs) {
    if (ptz::logShouldEmit(ptz::kLogRateHeartbeatFired, 2000)) {
      PTZ_LOGW("OSC", "heartbeat timeout (%ums since last rx), auto-stop",
               now - g_osc.lastRxMs());
    }
    g_motion.stop();
  }

  // Rate-limited heap monitoring (Pitfall 4 — CNMAT heap trend watch)
  if (ptz::logShouldEmit(ptz::kLogRateHeapFree, 60000)) {
    PTZ_LOGI("HEAP", "free=%u", ESP.getFreeHeap());
  }
}
