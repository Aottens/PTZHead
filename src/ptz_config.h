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

// --- OSC / Network ---
constexpr uint16_t kOscPort = 8000;
constexpr uint32_t kHeartbeatTimeoutMs = 500;

// --- Speed presets ---
struct SpeedPreset {
  float panScale;     // multiplier on kPanMaxSps
  float tiltScale;    // multiplier on kTiltMaxSps
  float zoomScale;    // multiplier on kZoomMaxSps
  float panAccel;     // absolute steps/sec^2
  float tiltAccel;    // absolute steps/sec^2
  float zoomAccel;    // absolute steps/sec^2
};

constexpr SpeedPreset kSpeedPresets[] = {
  // slow: 25% speed, 25% accel
  { 0.25f, 0.25f, 0.25f,  5000.0f,  5000.0f,  3750.0f },
  // medium (default): 60% speed, 60% accel
  { 0.60f, 0.60f, 0.60f, 12000.0f, 12000.0f,  9000.0f },
  // fast: 100% speed, 100% accel
  { 1.00f, 1.00f, 1.00f, 20000.0f, 20000.0f, 15000.0f },
};
constexpr uint8_t kDefaultPresetIndex = 1;
constexpr uint8_t kPresetCount = sizeof(kSpeedPresets) / sizeof(kSpeedPresets[0]);

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
  kLogRateOscRxVelocity = 1,
  kLogRateOscUnknownAddr = 2,
  kLogRateHeartbeatFired = 3,
  kLogRateOscParseError = 4,
  kLogRateWifiReconnect = 5,
  kLogRateHeapFree = 6,
  kLogRateCount = 7
};

}  // namespace ptz
