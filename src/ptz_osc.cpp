#include "ptz_osc.h"

#include <Arduino.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>

#include "ptz_config.h"
#include "ptz_log.h"
#include "ptz_motion.h"

namespace ptz {

// Module-scope state — CNMAT dispatch() takes raw function pointers, so capturing
// lambdas are impossible. Trampolines dereference these file-statics. See Pitfall 2.
static WiFiUDP s_udp;
static PtzOsc* s_self = nullptr;

namespace {

float clampNorm(float v) {
  if (v > 1.0f) return 1.0f;
  if (v < -1.0f) return -1.0f;
  return v;
}

void onPan(OSCMessage& m) {
  if (!s_self || !s_self->motionPtr()) return;
  if (!m.isFloat(0)) return;
  float v = clampNorm(m.getFloat(0));
  s_self->motionPtr()->setPanVelocity(v);
  if (logShouldEmit(kLogRateOscRxVelocity, 500)) {
    PTZ_LOGI("OSC", "pan=%.2f", v);
  }
}

void onTilt(OSCMessage& m) {
  if (!s_self || !s_self->motionPtr()) return;
  if (!m.isFloat(0)) return;
  float v = clampNorm(m.getFloat(0));
  s_self->motionPtr()->setTiltVelocity(v);
  if (logShouldEmit(kLogRateOscRxVelocity, 500)) {
    PTZ_LOGI("OSC", "tilt=%.2f", v);
  }
}

void onZoom(OSCMessage& m) {
  if (!s_self || !s_self->motionPtr()) return;
  if (!m.isFloat(0)) return;
  float v = clampNorm(m.getFloat(0));
  s_self->motionPtr()->setZoomVelocity(v);
  if (logShouldEmit(kLogRateOscRxVelocity, 500)) {
    PTZ_LOGI("OSC", "zoom=%.2f", v);
  }
}

void onStopAll(OSCMessage& /*m*/) {
  if (!s_self || !s_self->motionPtr()) return;
  s_self->motionPtr()->stop();
  PTZ_LOGI("OSC", "stop all");
}

void onStopPan(OSCMessage& /*m*/)  { if (s_self && s_self->motionPtr()) s_self->motionPtr()->stopPan(); }
void onStopTilt(OSCMessage& /*m*/) { if (s_self && s_self->motionPtr()) s_self->motionPtr()->stopTilt(); }
void onStopZoom(OSCMessage& /*m*/) { if (s_self && s_self->motionPtr()) s_self->motionPtr()->stopZoom(); }

void onPreset(OSCMessage& m) {
  if (!s_self || !s_self->motionPtr()) return;
  if (!m.isInt(0)) return;
  int32_t idx = m.getInt(0);
  if (idx < 0) idx = 0;
  if (idx >= static_cast<int32_t>(kPresetCount)) idx = kPresetCount - 1;
  s_self->motionPtr()->applySpeedPreset(static_cast<uint8_t>(idx));
  PTZ_LOGI("OSC", "preset=%d", idx);
}

void dispatchAll(OSCMessage& msg) {
  msg.dispatch("/ptz/pan",          onPan);
  msg.dispatch("/ptz/tilt",         onTilt);
  msg.dispatch("/ptz/zoom",         onZoom);
  msg.dispatch("/ptz/stop",         onStopAll);
  msg.dispatch("/ptz/pan/stop",     onStopPan);
  msg.dispatch("/ptz/tilt/stop",    onStopTilt);
  msg.dispatch("/ptz/zoom/stop",    onStopZoom);
  msg.dispatch("/ptz/speed/preset", onPreset);

  // Unknown-address detection: CNMAT dispatch() doesn't expose its internal
  // "dispatched" flag publicly in v3.5.8, so we check fullMatch against the
  // known set. If none match, log at rate-limited Debug level.
  if (!msg.fullMatch("/ptz/pan") && !msg.fullMatch("/ptz/tilt") &&
      !msg.fullMatch("/ptz/zoom") && !msg.fullMatch("/ptz/stop") &&
      !msg.fullMatch("/ptz/pan/stop") && !msg.fullMatch("/ptz/tilt/stop") &&
      !msg.fullMatch("/ptz/zoom/stop") && !msg.fullMatch("/ptz/speed/preset")) {
    if (logShouldEmit(kLogRateOscUnknownAddr, 2000)) {
      PTZ_LOGD("OSC", "unknown address");
    }
  }
}

}  // anonymous namespace

void PtzOsc::begin(PtzMotion* motion) {
  motion_ = motion;
  s_self = this;
  s_udp.begin(kOscPort);
  lastRxMs_ = millis();  // seed so heartbeat doesn't fire at boot
  PTZ_LOGI("OSC", "listening on UDP port %u", kOscPort);
}

void PtzOsc::update() {
  int size;
  while ((size = s_udp.parsePacket()) > 0) {
    OSCMessage msg;
    while (size--) msg.fill(s_udp.read());
    if (msg.hasError()) {
      if (logShouldEmit(kLogRateOscParseError, 1000)) {
        PTZ_LOGW("OSC", "parse error: %d", msg.getError());
      }
      continue;
    }
    lastRxMs_ = millis();
    dispatchAll(msg);
  }
}

}  // namespace ptz
