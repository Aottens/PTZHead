#pragma once
#include <stdint.h>
#include "FastAccelStepper.h"

namespace ptz {

class PtzMotion {
 public:
  void begin();
  void setVelocity(float panNorm, float tiltNorm, float zoomNorm);
  void stop();
  bool isMoving();
  int32_t panPosition();
  int32_t tiltPosition();
  int32_t zoomPosition();

  // Per-axis velocity (normalized -1.0..1.0); non-commanded axes are not touched.
  void setPanVelocity(float norm);
  void setTiltVelocity(float norm);
  void setZoomVelocity(float norm);

  // Per-axis stop (smooth deceleration).
  void stopPan();
  void stopTilt();
  void stopZoom();

  // Speed preset (index into kSpeedPresets). Out-of-range is clamped to kDefaultPresetIndex.
  // Rewrites max speed and acceleration for all 3 axes. If a stepper is currently running,
  // applySpeedAcceleration() is invoked on it so the new ramp takes effect immediately.
  void applySpeedPreset(uint8_t idx);
  uint8_t activePreset() const { return activePreset_; }

 private:
  void setAxisVelocity(FastAccelStepper* stepper, float norm, float maxSps);

  FastAccelStepperEngine engine_;
  FastAccelStepper* pan_ = nullptr;
  FastAccelStepper* tilt_ = nullptr;
  FastAccelStepper* zoom_ = nullptr;

  // Current effective max steps/sec per axis (base kPanMaxSps * preset.panScale, etc.)
  float panMaxSpsEff_ = 0.0f;
  float tiltMaxSpsEff_ = 0.0f;
  float zoomMaxSpsEff_ = 0.0f;
  uint8_t activePreset_ = 0;
};

}  // namespace ptz
