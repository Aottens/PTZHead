#pragma once
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

 private:
  void setAxisVelocity(FastAccelStepper* stepper, float norm, float maxSps);

  FastAccelStepperEngine engine_;
  FastAccelStepper* pan_ = nullptr;
  FastAccelStepper* tilt_ = nullptr;
  FastAccelStepper* zoom_ = nullptr;
};

}  // namespace ptz
