#include <Arduino.h>
#include <WiFi.h>

#include "ptz_config.h"
#include "ptz_log.h"
#include "ptz_motion.h"
#include "ptz_wifi.h"

namespace {

ptz::PtzMotion g_motion;
ptz::PtzWifi g_wifi;

void handleSerialCommands() {
  static String line;
  while (Serial.available()) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r') {
      line.trim();
      if (line.length() == 0) { line = ""; continue; }

      if (line.equalsIgnoreCase("WIFI RESET")) {
        g_wifi.resetAndProvision();
      }
      // Motor serial commands added in Plan 02 after FastAccelStepper migration
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

  PTZ_LOGI("BOOT", "Setup complete");
}

void loop() {
  handleSerialCommands();
}
