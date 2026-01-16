#pragma once

#include <Bluepad32.h>
#include <stdint.h>

namespace ptz {

struct GamepadCommands {
  float pan = 0.0f;
  float tilt = 0.0f;
  float zoom = 0.0f;
  bool hasInput = false;
  bool takeControl = false;
  bool provisioning = false;
  bool presetSave = false;
  bool presetRecall = false;
  uint8_t presetIndex = 0;
};

class PtzGamepad {
 public:
  void begin();
  void update();

  GamepadCommands readCommands(uint32_t nowMs);
  bool isConnected() const;
  void rumblePresetSaved();

 private:
  static void onConnect(GamepadPtr gp);
  static void onDisconnect(GamepadPtr gp);
  static GamepadPtr firstConnected();

  static GamepadPtr gamepads_[BP32_MAX_GAMEPADS];
  static uint32_t comboStartMs_;
  static uint32_t takeControlStartMs_;
  static uint32_t presetHoldStartMs_[4];
  static bool presetConsumed_[4];
  static bool presetPrevPressed_[4];
};

} // namespace ptz
