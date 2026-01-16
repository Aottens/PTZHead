#include "ptz_gamepad.h"

#include <Arduino.h>
#include <math.h>

#include "ptz_config.h"
#include "ptz_log.h"

namespace ptz {

GamepadPtr PtzGamepad::gamepads_[BP32_MAX_GAMEPADS] = {nullptr};
uint32_t PtzGamepad::comboStartMs_ = 0;
uint32_t PtzGamepad::takeControlStartMs_ = 0;
uint32_t PtzGamepad::presetHoldStartMs_[4] = {0, 0, 0, 0};
bool PtzGamepad::presetConsumed_[4] = {false, false, false, false};
bool PtzGamepad::presetPrevPressed_[4] = {false, false, false, false};

static float int16ToNorm(int16_t value) {
  float x = static_cast<float>(value) / 512.0f;
  if (x > 1.0f) {
    x = 1.0f;
  } else if (x < -1.0f) {
    x = -1.0f;
  }
  return x;
}

static float applyExpo(float x) {
  if (!kUseExpo) {
    return x;
  }
  return x * x * x;
}

static float applyDeadzone(float x) {
  if (fabsf(x) < kDeadzone) {
    return 0.0f;
  }
  const float sign = (x >= 0.0f) ? 1.0f : -1.0f;
  const float ax = fabsf(x);
  const float scaled = (ax - kDeadzone) / (1.0f - kDeadzone);
  return sign * applyExpo(scaled);
}

void PtzGamepad::begin() {
  BP32.setup(&PtzGamepad::onConnect, &PtzGamepad::onDisconnect);
}

void PtzGamepad::update() {
  BP32.update();
}

GamepadCommands PtzGamepad::readCommands(uint32_t nowMs) {
  GamepadCommands cmd;
  GamepadPtr gp = firstConnected();
  if (!gp) {
    comboStartMs_ = 0;
    takeControlStartMs_ = 0;
    for (int i = 0; i < 4; ++i) {
      presetHoldStartMs_[i] = 0;
      presetConsumed_[i] = false;
      presetPrevPressed_[i] = false;
    }
    return cmd;
  }

  cmd.pan = applyDeadzone(int16ToNorm(gp->axisX()));
  cmd.tilt = applyDeadzone(-int16ToNorm(gp->axisY()));
  cmd.zoom = applyDeadzone(-int16ToNorm(gp->axisRY()));
  if (kInvertPan) {
    cmd.pan = -cmd.pan;
  }
  cmd.hasInput = (cmd.pan != 0.0f) || (cmd.tilt != 0.0f) || (cmd.zoom != 0.0f);

  const bool presetPressed[4] = {gp->a(), gp->b(), gp->x(), gp->y()};
  for (uint8_t i = 0; i < 4; ++i) {
    if (presetPressed[i]) {
      if (!presetPrevPressed_[i]) {
        presetHoldStartMs_[i] = nowMs;
        presetConsumed_[i] = false;
      } else if (!presetConsumed_[i] && nowMs - presetHoldStartMs_[i] >= kPresetHoldMs) {
        cmd.presetSave = true;
        cmd.presetIndex = i;
        presetConsumed_[i] = true;
      }
    } else if (presetPrevPressed_[i]) {
      if (!presetConsumed_[i]) {
        cmd.presetRecall = true;
        cmd.presetIndex = i;
      }
      presetHoldStartMs_[i] = 0;
      presetConsumed_[i] = false;
    }
    presetPrevPressed_[i] = presetPressed[i];
  }

  const bool provisioningCombo = gp->l1() && gp->r1() && gp->x() && gp->y();
  if (provisioningCombo) {
    if (comboStartMs_ == 0) {
      comboStartMs_ = nowMs;
    }
    if (nowMs - comboStartMs_ >= kProvisionComboHoldMs) {
      cmd.provisioning = true;
      comboStartMs_ = 0;
    } else if (logShouldEmit(kLogRateGamepadCombo, 300)) {
      PTZ_LOGD("GAMEPAD", "Provision combo holding %lu ms", static_cast<unsigned long>(nowMs - comboStartMs_));
    }
  } else {
    comboStartMs_ = 0;
  }

  const bool takeCombo = gp->l1() && gp->r1() && gp->a();
  if (takeCombo) {
    if (takeControlStartMs_ == 0) {
      takeControlStartMs_ = nowMs;
    }
    if (nowMs - takeControlStartMs_ >= kTakeControlHoldMs) {
      cmd.takeControl = true;
      takeControlStartMs_ = 0;
    }
  } else {
    takeControlStartMs_ = 0;
  }

  return cmd;
}

bool PtzGamepad::isConnected() const {
  return firstConnected() != nullptr;
}

void PtzGamepad::rumblePresetSaved() {
  GamepadPtr gp = firstConnected();
  if (gp) {
    gp->setRumble(0xc0, 200);
  }
}

void PtzGamepad::onConnect(GamepadPtr gp) {
  for (int i = 0; i < BP32_MAX_GAMEPADS; ++i) {
    if (gamepads_[i] == nullptr) {
      gamepads_[i] = gp;
      PTZ_LOGI("GAMEPAD", "Connected index=%d", i);
      break;
    }
  }
}

void PtzGamepad::onDisconnect(GamepadPtr gp) {
  for (int i = 0; i < BP32_MAX_GAMEPADS; ++i) {
    if (gamepads_[i] == gp) {
      gamepads_[i] = nullptr;
      PTZ_LOGI("GAMEPAD", "Disconnected index=%d", i);
      break;
    }
  }
}

GamepadPtr PtzGamepad::firstConnected() {
  for (int i = 0; i < BP32_MAX_GAMEPADS; ++i) {
    if (gamepads_[i] && gamepads_[i]->isConnected()) {
      return gamepads_[i];
    }
  }
  return nullptr;
}

} // namespace ptz
