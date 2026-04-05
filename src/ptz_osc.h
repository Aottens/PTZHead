#pragma once
#include <stdint.h>
#include <IPAddress.h>

namespace ptz {

class PtzMotion;  // forward decl

struct StatusSnapshot {
  int32_t panMoving  = 0;
  int32_t tiltMoving = 0;
  int32_t zoomMoving = 0;
  int32_t preset     = -1;   // sentinel: force first emit after boot
  int32_t rssi       = 0;
};

class PtzOsc {
 public:
  // Starts UDP listener on kOscPort. Must be called AFTER WiFi is connected.
  // motion pointer must outlive this object (static g_motion in main.cpp).
  void begin(PtzMotion* motion);

  // Drain ALL queued UDP packets, dispatch each to handlers, update lastRxMs_.
  // Call every loop() iteration.
  void update();

  // Call every loop() iteration AFTER update(). Emits on-change status deltas
  // immediately and re-emits all 5 values every kFeedbackPeriodMs (1000ms) as
  // a self-heal snapshot. Silently no-ops if no RX packet has been received.
  void updateFeedback();

  // Returns true once any valid OSC packet has been received (sender cache populated).
  bool hasSender() const { return lastSenderPort_ != 0; }

  // millis() timestamp of most recent valid OSC packet (any address).
  // Returns 0 if none received yet.
  uint32_t lastRxMs() const { return lastRxMs_; }

  // Accessor used by file-static trampolines to reach motion_ via s_self.
  PtzMotion* motionPtr() { return motion_; }

 private:
  PtzMotion* motion_ = nullptr;
  uint32_t lastRxMs_ = 0;

  // Reply-to-sender cache — populated from s_udp.remoteIP()/remotePort() after
  // each successful packet parse. NEVER touched by the send path.
  IPAddress lastSenderIp_;
  uint16_t  lastSenderPort_ = 0;

  // Feedback TX state
  uint32_t       lastFeedbackMs_ = 0;
  StatusSnapshot lastSent_;

  // Helpers
  void sendScalarInt(const char* addr, int32_t value);
};

}  // namespace ptz
