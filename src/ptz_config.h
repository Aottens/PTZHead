#pragma once

#include <stdint.h>

namespace ptz {

constexpr uint8_t kPanStepPin = 16;
constexpr uint8_t kPanDirPin = 17;
constexpr uint8_t kPanEnPin = 25; // active low

constexpr uint8_t kTiltStepPin = 18;
constexpr uint8_t kTiltDirPin = 19;
constexpr uint8_t kTiltEnPin = 26; // active low

constexpr uint8_t kZoomStepPin = 22;
constexpr uint8_t kZoomDirPin = 23;
constexpr uint8_t kZoomEnPin = 27; // active low

constexpr float kDeadzone = 0.08f;
constexpr bool kUseExpo = true;

constexpr float kPanMaxSps = 8000.0f;
constexpr float kTiltMaxSps = 8000.0f;
constexpr float kZoomMaxSps = 6000.0f;

constexpr float kPanAccel = 20000.0f;
constexpr float kTiltAccel = 20000.0f;
constexpr float kZoomAccel = 15000.0f;

constexpr uint32_t kStatusIntervalMs = 50;
constexpr uint32_t kAppHeartbeatTimeoutMs = 750;
constexpr uint32_t kIdleDisableTimeoutMs = 2000;
constexpr uint32_t kGamepadOwnerTimeoutMs = 1000;

constexpr uint32_t kWifiConnectTimeoutS = 20;
constexpr uint32_t kWifiPortalTimeoutS = 180;
constexpr const char* kWifiApName = "PTZHead Setup";
constexpr const char* kWifiApPass = "";

constexpr uint16_t kWebsocketPort = 81;
constexpr const char* kWebsocketPath = "/ws";

constexpr uint8_t kProtocolVersion = 1;

constexpr uint32_t kProvisionComboHoldMs = 2000;
constexpr uint32_t kTakeControlHoldMs = 1000;

enum class LogLevel : uint8_t {
  Error = 0,
  Warn = 1,
  Info = 2,
  Debug = 3,
};

constexpr LogLevel kLogLevel = LogLevel::Info;

enum LogRateId : uint8_t {
  kLogRateWifiProgress = 0,
  kLogRateWsStatus = 1,
  kLogRateWsParseError = 2,
  kLogRateGamepadCombo = 3,
  kLogRateOwnerState = 4,
  kLogRateCount = 8
};

} // namespace ptz
