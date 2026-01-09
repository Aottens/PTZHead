#pragma once

#include <AccelStepper.h>

namespace ptz {

struct MotionState {
  float panPos;
  float tiltPos;
  float zoomPos;
  float panTarget;
  float tiltTarget;
  float zoomTarget;
};

class PtzMotion {
 public:
  PtzMotion();

  void begin();
  void update(float dtSeconds);
  void run();

  void setVelocity(float panNorm, float tiltNorm, float zoomNorm);
  void moveTo(float panSteps, float tiltSteps, float zoomSteps);
  void stop();

  void setEnabled(bool enabled);
  bool enabled() const;
  bool isMoving() const;

  MotionState state() const;

 private:
  AccelStepper pan_;
  AccelStepper tilt_;
  AccelStepper zoom_;

  float panTarget_ = 0.0f;
  float tiltTarget_ = 0.0f;
  float zoomTarget_ = 0.0f;

  float panVelocity_ = 0.0f;
  float tiltVelocity_ = 0.0f;
  float zoomVelocity_ = 0.0f;

  bool outputsEnabled_ = false;
};

} // namespace ptz
