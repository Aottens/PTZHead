#pragma once

#include <WebSocketsServer.h>

#include "ptz_motion.h"
#include "ptz_owner.h"

namespace ptz {

class PtzWebSocket {
 public:
  PtzWebSocket();

  void begin(PtzOwner* owner, PtzMotion* motion);
  void loop();

  void broadcastStatus(uint32_t nowMs,
                       PtzMotion& motion,
                       const PtzOwner& owner,
                       bool gamepadConnected,
                       bool motorsEnabled,
                       int wifiRssi);

 private:
  void onEvent(uint8_t clientNum,
               WStype_t type,
               uint8_t* payload,
               size_t length);

  void handleText(uint8_t clientNum, const char* payload, size_t len);
  void sendAck(uint8_t clientNum, const char* refType, uint32_t nowMs);
  void sendError(uint8_t clientNum,
                 const char* code,
                 const char* message,
                 uint32_t nowMs);

  WebSocketsServer ws_;
  PtzOwner* owner_ = nullptr;
  PtzMotion* motion_ = nullptr;
};

} // namespace ptz
