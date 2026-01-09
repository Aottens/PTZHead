#include "ptz_log.h"

#include <stdarg.h>

namespace ptz {

struct RateEntry {
  uint32_t lastMs = 0;
};

static RateEntry s_rateEntries[kLogRateCount];

void logInit() {
  Serial.flush();
}

bool logShouldEmit(uint8_t id, uint32_t intervalMs) {
  if (id >= kLogRateCount) {
    return true;
  }
  const uint32_t now = millis();
  const uint32_t last = s_rateEntries[id].lastMs;
  if (now - last >= intervalMs) {
    s_rateEntries[id].lastMs = now;
    return true;
  }
  return false;
}

void logMessage(LogLevel level, const char* tag, const char* fmt, ...) {
  static const char* kLevelNames[] = {"E", "W", "I", "D"};
  const uint8_t levelIndex = static_cast<uint8_t>(level);
  if (levelIndex > 3) {
    return;
  }

  char buffer[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);

  Serial.printf("[%s] %s | %s\n", kLevelNames[levelIndex], tag, buffer);
}

} // namespace ptz
