#pragma once
#include <stdint.h>

namespace ptz {

class PtzMotion;  // forward decl

class PtzOsc {
 public:
  // Starts UDP listener on kOscPort. Must be called AFTER WiFi is connected.
  // motion pointer must outlive this object (static g_motion in main.cpp).
  void begin(PtzMotion* motion);

  // Drain ALL queued UDP packets, dispatch each to handlers, update lastRxMs_.
  // Call every loop() iteration.
  void update();

  // millis() timestamp of most recent valid OSC packet (any address).
  // Returns 0 if none received yet.
  uint32_t lastRxMs() const { return lastRxMs_; }

  // Accessor used by file-static trampolines to reach motion_ via s_self.
  PtzMotion* motionPtr() { return motion_; }

 private:
  PtzMotion* motion_ = nullptr;
  uint32_t lastRxMs_ = 0;
};

}  // namespace ptz
