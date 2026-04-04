#include "ptz_motion.h"
#include <Arduino.h>
#include <math.h>
#include "ptz_config.h"
#include "ptz_log.h"

namespace ptz {

void PtzMotion::begin() {
  engine_.init();

  pan_ = engine_.stepperConnectToPin(kPanStepPin);
  if (!pan_) { PTZ_LOGE("MOTION", "Failed to init pan stepper"); return; }
  pan_->setDirectionPin(kPanDirPin, !kInvertPan);
  pan_->setEnablePin(kPanEnPin, true);
  pan_->setAutoEnable(true);
  pan_->setDelayToDisable(500);
  pan_->setAcceleration(static_cast<int32_t>(kPanAccel));

  tilt_ = engine_.stepperConnectToPin(kTiltStepPin);
  if (!tilt_) { PTZ_LOGE("MOTION", "Failed to init tilt stepper"); return; }
  tilt_->setDirectionPin(kTiltDirPin);
  tilt_->setEnablePin(kTiltEnPin, true);
  tilt_->setAutoEnable(true);
  tilt_->setDelayToDisable(500);
  tilt_->setAcceleration(static_cast<int32_t>(kTiltAccel));

  zoom_ = engine_.stepperConnectToPin(kZoomStepPin);
  if (!zoom_) { PTZ_LOGE("MOTION", "Failed to init zoom stepper"); return; }
  zoom_->setDirectionPin(kZoomDirPin);
  zoom_->setEnablePin(kZoomEnPin, true);
  zoom_->setAutoEnable(true);
  zoom_->setDelayToDisable(500);
  zoom_->setAcceleration(static_cast<int32_t>(kZoomAccel));

  PTZ_LOGI("MOTION", "3-axis FastAccelStepper initialized");
}

void PtzMotion::setAxisVelocity(FastAccelStepper* stepper, float norm, float maxSps) {
  if (!stepper) return;
  if (fabsf(norm) < 0.001f) {
    stepper->stopMove();
    return;
  }
  uint32_t speedHz = static_cast<uint32_t>(fabsf(norm) * maxSps);
  if (speedHz < 1) speedHz = 1;
  stepper->setSpeedInHz(speedHz);
  stepper->applySpeedAcceleration();
  if (norm > 0.0f) {
    stepper->runForward();
  } else {
    stepper->runBackward();
  }
}

void PtzMotion::setVelocity(float panNorm, float tiltNorm, float zoomNorm) {
  setAxisVelocity(pan_, panNorm, kPanMaxSps);
  setAxisVelocity(tilt_, tiltNorm, kTiltMaxSps);
  setAxisVelocity(zoom_, zoomNorm, kZoomMaxSps);
}

void PtzMotion::stop() {
  if (pan_) pan_->stopMove();
  if (tilt_) tilt_->stopMove();
  if (zoom_) zoom_->stopMove();
}

bool PtzMotion::isMoving() {
  return (pan_ && pan_->isRunning()) ||
         (tilt_ && tilt_->isRunning()) ||
         (zoom_ && zoom_->isRunning());
}

int32_t PtzMotion::panPosition() {
  return pan_ ? pan_->getCurrentPosition() : 0;
}

int32_t PtzMotion::tiltPosition() {
  return tilt_ ? tilt_->getCurrentPosition() : 0;
}

int32_t PtzMotion::zoomPosition() {
  return zoom_ ? zoom_->getCurrentPosition() : 0;
}

}  // namespace ptz
