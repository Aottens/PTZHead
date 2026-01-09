#include "ptz_motion.h"

#include <Arduino.h>
#include <math.h>

#include "ptz_config.h"

namespace ptz {

PtzMotion::PtzMotion()
    : pan_(AccelStepper::DRIVER, kPanStepPin, kPanDirPin),
      tilt_(AccelStepper::DRIVER, kTiltStepPin, kTiltDirPin),
      zoom_(AccelStepper::DRIVER, kZoomStepPin, kZoomDirPin) {}

void PtzMotion::begin() {
  pan_.setMaxSpeed(kPanMaxSps);
  pan_.setAcceleration(kPanAccel);
  pan_.setEnablePin(kPanEnPin);
  pan_.setPinsInverted(false, false, true);

  tilt_.setMaxSpeed(kTiltMaxSps);
  tilt_.setAcceleration(kTiltAccel);
  tilt_.setEnablePin(kTiltEnPin);
  tilt_.setPinsInverted(false, false, true);

  zoom_.setMaxSpeed(kZoomMaxSps);
  zoom_.setAcceleration(kZoomAccel);
  zoom_.setEnablePin(kZoomEnPin);
  zoom_.setPinsInverted(false, false, true);

  setEnabled(false);
}

void PtzMotion::update(float dtSeconds) {
  panTarget_ += panVelocity_ * dtSeconds;
  tiltTarget_ += tiltVelocity_ * dtSeconds;
  zoomTarget_ += zoomVelocity_ * dtSeconds;

  pan_.moveTo(lroundf(panTarget_));
  tilt_.moveTo(lroundf(tiltTarget_));
  zoom_.moveTo(lroundf(zoomTarget_));
}

void PtzMotion::run() {
  pan_.run();
  tilt_.run();
  zoom_.run();
}

void PtzMotion::setVelocity(float panNorm, float tiltNorm, float zoomNorm) {
  panVelocity_ = panNorm * kPanMaxSps;
  tiltVelocity_ = tiltNorm * kTiltMaxSps;
  zoomVelocity_ = zoomNorm * kZoomMaxSps;
}

void PtzMotion::moveTo(float panSteps, float tiltSteps, float zoomSteps) {
  panTarget_ = panSteps;
  tiltTarget_ = tiltSteps;
  zoomTarget_ = zoomSteps;

  pan_.moveTo(lroundf(panTarget_));
  tilt_.moveTo(lroundf(tiltTarget_));
  zoom_.moveTo(lroundf(zoomTarget_));
}

void PtzMotion::stop() {
  panTarget_ = static_cast<float>(pan_.currentPosition());
  tiltTarget_ = static_cast<float>(tilt_.currentPosition());
  zoomTarget_ = static_cast<float>(zoom_.currentPosition());

  panVelocity_ = 0.0f;
  tiltVelocity_ = 0.0f;
  zoomVelocity_ = 0.0f;

  pan_.moveTo(lroundf(panTarget_));
  tilt_.moveTo(lroundf(tiltTarget_));
  zoom_.moveTo(lroundf(zoomTarget_));
}

void PtzMotion::setEnabled(bool enabled) {
  outputsEnabled_ = enabled;
  if (enabled) {
    pan_.enableOutputs();
    tilt_.enableOutputs();
    zoom_.enableOutputs();
  } else {
    pan_.disableOutputs();
    tilt_.disableOutputs();
    zoom_.disableOutputs();
  }
}

bool PtzMotion::enabled() const {
  return outputsEnabled_;
}

bool PtzMotion::isMoving() {
  return pan_.distanceToGo() != 0 || tilt_.distanceToGo() != 0 || zoom_.distanceToGo() != 0;
}

MotionState PtzMotion::state() {
  MotionState state{
      static_cast<float>(pan_.currentPosition()),
      static_cast<float>(tilt_.currentPosition()),
      static_cast<float>(zoom_.currentPosition()),
      panTarget_,
      tiltTarget_,
      zoomTarget_,
  };
  return state;
}

} // namespace ptz
