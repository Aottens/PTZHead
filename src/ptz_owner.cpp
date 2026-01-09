#include "ptz_owner.h"

#include "ptz_config.h"

namespace ptz {

void PtzOwner::begin() {
  owner_ = Owner::None;
  controlClientId_ = 0;
  lastAppHeartbeatMs_ = 0;
  lastGamepadActiveMs_ = 0;
  lastOwnerChangeMs_ = 0;
}

OwnerSnapshot PtzOwner::snapshot() const {
  return OwnerSnapshot{owner_, controlClientId_};
}

Owner PtzOwner::owner() const {
  return owner_;
}

bool PtzOwner::requestAppControl(uint32_t clientId, uint32_t nowMs) {
  controlClientId_ = clientId;
  lastAppHeartbeatMs_ = nowMs;
  setOwner(Owner::App, nowMs);
  return true;
}

bool PtzOwner::releaseAppControl(uint32_t clientId) {
  if (owner_ != Owner::App || controlClientId_ != clientId) {
    return false;
  }
  controlClientId_ = 0;
  owner_ = Owner::None;
  return true;
}

void PtzOwner::appHeartbeat(uint32_t clientId, uint32_t nowMs) {
  if (owner_ != Owner::App) {
    return;
  }
  if (controlClientId_ != clientId) {
    return;
  }
  lastAppHeartbeatMs_ = nowMs;
}

void PtzOwner::setGamepadActive(uint32_t nowMs) {
  lastGamepadActiveMs_ = nowMs;
  if (owner_ == Owner::Gamepad) {
    return;
  }
}

bool PtzOwner::requestGamepadControl(uint32_t nowMs) {
  lastGamepadActiveMs_ = nowMs;
  setOwner(Owner::Gamepad, nowMs);
  return true;
}

void PtzOwner::clearGamepad() {
  if (owner_ == Owner::Gamepad) {
    owner_ = Owner::None;
  }
  lastGamepadActiveMs_ = 0;
}

bool PtzOwner::update(uint32_t nowMs) {
  const Owner previous = owner_;

  if (owner_ == Owner::App) {
    if (nowMs - lastAppHeartbeatMs_ > kAppHeartbeatTimeoutMs) {
      owner_ = Owner::None;
      controlClientId_ = 0;
    }
  } else if (owner_ == Owner::Gamepad) {
    if (nowMs - lastGamepadActiveMs_ > kGamepadOwnerTimeoutMs) {
      owner_ = Owner::None;
    }
  }

  if (owner_ == Owner::None && lastOwnerChangeMs_ == 0) {
    lastOwnerChangeMs_ = nowMs;
  }

  return previous != owner_;
}

void PtzOwner::setOwner(Owner owner, uint32_t nowMs) {
  owner_ = owner;
  lastOwnerChangeMs_ = nowMs;
}

} // namespace ptz
