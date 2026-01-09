#pragma once

#include <stdint.h>

namespace ptz {

enum class Owner : uint8_t {
  None = 0,
  App = 1,
  Gamepad = 2,
};

struct OwnerSnapshot {
  Owner owner;
  uint32_t controlClientId;
};

class PtzOwner {
 public:
  void begin();

  OwnerSnapshot snapshot() const;
  Owner owner() const;

  bool requestAppControl(uint32_t clientId, uint32_t nowMs);
  bool releaseAppControl(uint32_t clientId);
  void appHeartbeat(uint32_t clientId, uint32_t nowMs);

  void setGamepadActive(uint32_t nowMs);
  bool requestGamepadControl(uint32_t nowMs);
  void clearGamepad();

  bool update(uint32_t nowMs);

 private:
  void setOwner(Owner owner, uint32_t nowMs);

  Owner owner_ = Owner::None;
  uint32_t controlClientId_ = 0;
  uint32_t lastAppHeartbeatMs_ = 0;
  uint32_t lastGamepadActiveMs_ = 0;
  uint32_t lastOwnerChangeMs_ = 0;
};

} // namespace ptz
