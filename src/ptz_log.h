#pragma once

#include <Arduino.h>
#include "ptz_config.h"

namespace ptz {

void logInit();

bool logShouldEmit(uint8_t id, uint32_t intervalMs);

void logMessage(LogLevel level, const char* tag, const char* fmt, ...);

#define PTZ_LOGE(tag, fmt, ...) \
  do { \
    if (ptz::kLogLevel >= ptz::LogLevel::Error) { \
      ptz::logMessage(ptz::LogLevel::Error, tag, fmt, ##__VA_ARGS__); \
    } \
  } while (0)

#define PTZ_LOGW(tag, fmt, ...) \
  do { \
    if (ptz::kLogLevel >= ptz::LogLevel::Warn) { \
      ptz::logMessage(ptz::LogLevel::Warn, tag, fmt, ##__VA_ARGS__); \
    } \
  } while (0)

#define PTZ_LOGI(tag, fmt, ...) \
  do { \
    if (ptz::kLogLevel >= ptz::LogLevel::Info) { \
      ptz::logMessage(ptz::LogLevel::Info, tag, fmt, ##__VA_ARGS__); \
    } \
  } while (0)

#define PTZ_LOGD(tag, fmt, ...) \
  do { \
    if (ptz::kLogLevel >= ptz::LogLevel::Debug) { \
      ptz::logMessage(ptz::LogLevel::Debug, tag, fmt, ##__VA_ARGS__); \
    } \
  } while (0)

} // namespace ptz
