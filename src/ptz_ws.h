#pragma once

#include <ESPAsyncWebServer.h>

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
  void onEvent(AsyncWebSocket* server,
               AsyncWebSocketClient* client,
               AwsEventType type,
               void* arg,
               uint8_t* data,
               size_t len);

  void handleText(AsyncWebSocketClient* client, const char* payload, size_t len);
  void sendAck(AsyncWebSocketClient* client, const char* refType, uint32_t nowMs);
  void sendError(AsyncWebSocketClient* client,
                 const char* code,
                 const char* message,
                 uint32_t nowMs);

  AsyncWebServer server_;
  AsyncWebSocket ws_;
  PtzOwner* owner_ = nullptr;
  PtzMotion* motion_ = nullptr;
};

} // namespace ptz
