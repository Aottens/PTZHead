#include <Arduino.h>
#include <WiFi.h>

#include "ptz_config.h"
#include "ptz_gamepad.h"
#include "ptz_log.h"
#include "ptz_motion.h"
#include "ptz_owner.h"
#include "ptz_wifi.h"
#include "ptz_ws.h"

using ptz::Owner;

namespace {

ptz::PtzGamepad g_gamepad;
ptz::PtzMotion g_motion;
ptz::PtzOwner g_owner;
ptz::PtzWifi g_wifi;
ptz::PtzWebSocket g_ws;

uint32_t g_lastMicros = 0;
uint32_t g_lastStatusMs = 0;
uint32_t g_idleStartMs = 0;
Owner g_lastOwner = Owner::None;

void handleSerialCommands() {
  static String line;
  while (Serial.available()) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r') {
      line.trim();
      if (line.equalsIgnoreCase("WIFI RESET")) {
        g_wifi.resetAndProvision();
      }
      line = "";
    } else {
      line += c;
    }
  }
}

float clampNorm(float x) {
  if (x > 1.0f) {
    return 1.0f;
  }
  if (x < -1.0f) {
    return -1.0f;
  }
  return x;
}

} // namespace

void setup() {
  Serial.begin(115200);
  delay(200);
  ptz::logInit();

  PTZ_LOGI("BOOT", "PTZHead starting");

  g_owner.begin();
  g_motion.begin();
  g_gamepad.begin();

  g_wifi.begin(false);
  g_ws.begin(&g_owner, &g_motion);

  g_lastMicros = micros();
  g_lastStatusMs = millis();
  g_lastOwner = g_owner.owner();
}

void loop() {
  const uint32_t nowMs = millis();
  const uint32_t nowUs = micros();
  float dt = (nowUs - g_lastMicros) * 1e-6f;
  if (dt < 0.0f) {
    dt = 0.0f;
  } else if (dt > 0.05f) {
    dt = 0.05f;
  }
  g_lastMicros = nowUs;

  g_gamepad.update();
  g_ws.loop();
  handleSerialCommands();

  ptz::GamepadCommands commands = g_gamepad.readCommands(nowMs);

  if (commands.provisioning) {
    PTZ_LOGW("WIFI", "Provisioning combo triggered");
    g_wifi.resetAndProvision();
  }

  if (commands.takeControl) {
    g_owner.requestGamepadControl(nowMs);
  }

  if (commands.hasInput) {
    g_owner.setGamepadActive(nowMs);
    if (g_owner.owner() == Owner::None) {
      g_owner.requestGamepadControl(nowMs);
    }
  }

  if (!g_gamepad.isConnected()) {
    g_owner.clearGamepad();
  }

  g_owner.update(nowMs);
  const Owner currentOwner = g_owner.owner();
  if (currentOwner != g_lastOwner) {
    PTZ_LOGI("OWNER", "Owner changed to %u", static_cast<unsigned>(currentOwner));
    if (currentOwner == Owner::None) {
      g_motion.stop();
    }
    g_lastOwner = currentOwner;
  }

  if (currentOwner == Owner::Gamepad) {
    g_motion.setVelocity(clampNorm(commands.pan), clampNorm(commands.tilt), clampNorm(commands.zoom));
  }

  g_motion.update(dt);
  g_motion.run();

  if (currentOwner != Owner::None) {
    if (!g_motion.enabled()) {
      g_motion.setEnabled(true);
    }
    g_idleStartMs = 0;
  }

  if (currentOwner == Owner::None && !g_motion.isMoving()) {
    if (g_idleStartMs == 0) {
      g_idleStartMs = nowMs;
    }
    if (nowMs - g_idleStartMs >= ptz::kIdleDisableTimeoutMs) {
      if (g_motion.enabled()) {
        g_motion.setEnabled(false);
        PTZ_LOGI("MOTION", "Idle timeout, outputs disabled");
      }
    }
  } else {
    g_idleStartMs = 0;
  }

  if (nowMs - g_lastStatusMs >= ptz::kStatusIntervalMs) {
    const int wifiRssi = (WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : 0;
    g_ws.broadcastStatus(nowMs, g_motion, g_owner, g_gamepad.isConnected(), g_motion.enabled(), wifiRssi);
    g_lastStatusMs = nowMs;
  }
}
