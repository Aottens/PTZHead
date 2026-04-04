#pragma once
#include <stdint.h>

namespace ptz {

// --- Pin assignments (unchanged) ---
constexpr uint8_t kPanStepPin = 16;
constexpr uint8_t kPanDirPin = 17;
constexpr uint8_t kPanEnPin = 25;  // active low

constexpr uint8_t kTiltStepPin = 18;
constexpr uint8_t kTiltDirPin = 19;
constexpr uint8_t kTiltEnPin = 26;  // active low

constexpr uint8_t kZoomStepPin = 22;
constexpr uint8_t kZoomDirPin = 23;
constexpr uint8_t kZoomEnPin = 27;  // active low

// --- Motion parameters ---
constexpr bool kInvertPan = true;

constexpr float kPanMaxSps = 4000.0f;
constexpr float kTiltMaxSps = 4000.0f;
constexpr float kZoomMaxSps = 4000.0f;

constexpr float kPanAccel = 20000.0f;
constexpr float kTiltAccel = 20000.0f;
constexpr float kZoomAccel = 15000.0f;

// --- WiFi ---
constexpr uint32_t kWifiConnectTimeoutS = 20;
constexpr uint32_t kWifiPortalTimeoutS = 180;
constexpr const char* kWifiApName = "PTZHead Setup";
constexpr const char* kWifiApPass = "";

// --- Logging ---
enum class LogLevel : uint8_t {
  Error = 0,
  Warn = 1,
  Info = 2,
  Debug = 3,
};

constexpr LogLevel kLogLevel = LogLevel::Info;

enum LogRateId : uint8_t {
  kLogRateWifiProgress = 0,
  kLogRateCount = 4
};

}  // namespace ptz
