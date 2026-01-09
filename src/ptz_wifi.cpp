#include "ptz_wifi.h"

#include <WiFi.h>
#include <WiFiManager.h>
#include <esp_wifi.h>

#include "ptz_config.h"
#include "ptz_log.h"

namespace ptz {

static void printWifiStatus() {
  if (WiFi.status() == WL_CONNECTED) {
    PTZ_LOGI("WIFI", "Connected SSID=%s IP=%s RSSI=%d",
             WiFi.SSID().c_str(),
             WiFi.localIP().toString().c_str(),
             WiFi.RSSI());
  } else {
    PTZ_LOGW("WIFI", "WiFi status=%d", static_cast<int>(WiFi.status()));
  }
}

static void startProvisioningPortal(WiFiManager& wm) {
  PTZ_LOGI("WIFI", "Starting provisioning portal");

  wm.setConfigPortalTimeout(kWifiPortalTimeoutS);
  wm.setConnectTimeout(kWifiConnectTimeoutS);

  wm.setAPCallback([](WiFiManager* w) {
    PTZ_LOGI("WIFI", "AP started SSID=%s IP=%s",
             w->getConfigPortalSSID().c_str(),
             WiFi.softAPIP().toString().c_str());
  });

  wm.setSaveConfigCallback([]() { PTZ_LOGI("WIFI", "Credentials saved"); });

  bool ok = false;
  if (strlen(kWifiApPass) == 0) {
    ok = wm.startConfigPortal(kWifiApName);
  } else {
    ok = wm.startConfigPortal(kWifiApName, kWifiApPass);
  }

  if (ok && WiFi.status() == WL_CONNECTED) {
    PTZ_LOGI("WIFI", "Provisioning success");
    printWifiStatus();
    return;
  }

  PTZ_LOGW("WIFI", "Provisioning timed out, restarting");
  delay(300);
  ESP.restart();
}

static bool tryStoredCredentials() {
  PTZ_LOGI("WIFI", "Trying stored credentials");

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(true);
  esp_wifi_set_ps(WIFI_PS_MIN_MODEM);

  WiFi.begin();

  const uint32_t startMs = millis();
  while (millis() - startMs < (kWifiConnectTimeoutS * 1000UL)) {
    if (WiFi.status() == WL_CONNECTED) {
      PTZ_LOGI("WIFI", "Connected via stored credentials");
      printWifiStatus();
      return true;
    }
    if (logShouldEmit(kLogRateWifiProgress, 500)) {
      PTZ_LOGI("WIFI", "Connecting status=%d", static_cast<int>(WiFi.status()));
    }
    delay(10);
  }

  PTZ_LOGW("WIFI", "Connect timeout");
  return false;
}

void PtzWifi::begin(bool forcePortal) {
  WiFiManager wm;
  wm.setDebugOutput(false);

  if (forcePortal) {
    startProvisioningPortal(wm);
    return;
  }

  if (tryStoredCredentials()) {
    return;
  }

  startProvisioningPortal(wm);
}

void PtzWifi::resetAndProvision() {
  PTZ_LOGW("WIFI", "Resetting credentials");
  WiFiManager wm;
  wm.setDebugOutput(false);
  wm.resetSettings();
  delay(200);
  begin(true);
}

} // namespace ptz
