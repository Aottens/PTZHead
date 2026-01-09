#include "ptz_ws.h"

#include <ArduinoJson.h>
#include "ptz_config.h"
#include "ptz_log.h"

namespace ptz {

static float clampNorm(float value) {
  if (value > 1.0f) {
    return 1.0f;
  }
  if (value < -1.0f) {
    return -1.0f;
  }
  return value;
}

PtzWebSocket::PtzWebSocket() : server_(kWebsocketPort), ws_(kWebsocketPath) {}

void PtzWebSocket::begin(PtzOwner* owner, PtzMotion* motion) {
  owner_ = owner;
  motion_ = motion;

  ws_.onEvent([this](AsyncWebSocket* server,
                     AsyncWebSocketClient* client,
                     AwsEventType type,
                     void* arg,
                     uint8_t* data,
                     size_t len) {
    onEvent(server, client, type, arg, data, len);
  });

  server_.addHandler(&ws_);
  server_.begin();
  PTZ_LOGI("WS", "WebSocket listening on port %u path %s", kWebsocketPort, kWebsocketPath);
}

void PtzWebSocket::loop() {
  ws_.cleanupClients();
}

void PtzWebSocket::broadcastStatus(uint32_t nowMs,
                                   const PtzMotion& motion,
                                   const PtzOwner& owner,
                                   bool gamepadConnected,
                                   bool motorsEnabled,
                                   int wifiRssi) {
  StaticJsonDocument<768> doc;
  doc["v"] = kProtocolVersion;
  doc["type"] = "status";
  doc["timestampMs"] = nowMs;

  const OwnerSnapshot snap = owner.snapshot();
  const char* ownerLabel = "none";
  if (snap.owner == Owner::App) {
    ownerLabel = "app";
  } else if (snap.owner == Owner::Gamepad) {
    ownerLabel = "gamepad";
  }
  doc["owner"] = ownerLabel;
  doc["wifiRssi"] = wifiRssi;
  doc["gamepadConnected"] = gamepadConnected;
  doc["motorsEnabled"] = motorsEnabled;

  const MotionState state = motion.state();
  JsonObject pan = doc.createNestedObject("pan");
  pan["pos"] = state.panPos;
  pan["target"] = state.panTarget;

  JsonObject tilt = doc.createNestedObject("tilt");
  tilt["pos"] = state.tiltPos;
  tilt["target"] = state.tiltTarget;

  JsonObject zoom = doc.createNestedObject("zoom");
  zoom["pos"] = state.zoomPos;
  zoom["target"] = state.zoomTarget;

  char buffer[512];
  const size_t size = serializeJson(doc, buffer, sizeof(buffer));
  ws_.textAll(buffer, size);

  if (logShouldEmit(kLogRateWsStatus, 1000)) {
    PTZ_LOGD("WS", "Status broadcast %u bytes", static_cast<unsigned>(size));
  }
}

void PtzWebSocket::onEvent(AsyncWebSocket* server,
                           AsyncWebSocketClient* client,
                           AwsEventType type,
                           void* arg,
                           uint8_t* data,
                           size_t len) {
  (void)server;
  if (type == WS_EVT_CONNECT) {
    PTZ_LOGI("WS", "Client connected id=%u", client->id());
  } else if (type == WS_EVT_DISCONNECT) {
    PTZ_LOGI("WS", "Client disconnected id=%u", client->id());
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo* info = reinterpret_cast<AwsFrameInfo*>(arg);
    if (info->opcode == WS_TEXT && info->final && info->index == 0 && info->len == len) {
      handleText(client, reinterpret_cast<char*>(data), len);
    }
  }
}

void PtzWebSocket::handleText(AsyncWebSocketClient* client, const char* payload, size_t len) {
  StaticJsonDocument<384> doc;
  DeserializationError err = deserializeJson(doc, payload, len);
  const uint32_t nowMs = millis();

  if (err) {
    if (logShouldEmit(kLogRateWsParseError, 500)) {
      PTZ_LOGW("WS", "JSON parse error: %s", err.c_str());
    }
    sendError(client, "invalid_json", "Failed to parse JSON", nowMs);
    return;
  }

  const uint8_t version = doc["v"] | 0;
  const char* type = doc["type"] | "";
  const char* source = doc["source"] | "";

  if (version != kProtocolVersion) {
    sendError(client, "invalid_version", "Unsupported protocol version", nowMs);
    return;
  }

  if (strcmp(source, "app") != 0) {
    sendError(client, "invalid_source", "Only app source supported", nowMs);
    return;
  }

  const uint32_t clientId = client->id();

  if (strcmp(type, "requestControl") == 0) {
    owner_->requestAppControl(clientId, nowMs);
    sendAck(client, "requestControl", nowMs);
    PTZ_LOGI("OWNER", "App requested control client=%u", clientId);
    return;
  }

  if (strcmp(type, "releaseControl") == 0) {
    if (!owner_->releaseAppControl(clientId)) {
      sendError(client, "not_owner", "Client is not the active owner", nowMs);
      return;
    }
    sendAck(client, "releaseControl", nowMs);
    PTZ_LOGI("OWNER", "App released control client=%u", clientId);
    return;
  }

  const OwnerSnapshot snap = owner_->snapshot();
  if (snap.owner != Owner::App || snap.controlClientId != clientId) {
    sendError(client, "not_owner", "Client is not the active owner", nowMs);
    return;
  }

  owner_->appHeartbeat(clientId, nowMs);

  if (strcmp(type, "setVelocity") == 0) {
    if (!doc["pan"].is<float>() || !doc["tilt"].is<float>() || !doc["zoom"].is<float>()) {
      sendError(client, "invalid_payload", "Missing velocity fields", nowMs);
      return;
    }
    const float pan = clampNorm(doc["pan"].as<float>());
    const float tilt = clampNorm(doc["tilt"].as<float>());
    const float zoom = clampNorm(doc["zoom"].as<float>());
    motion_->setVelocity(pan, tilt, zoom);
    sendAck(client, "setVelocity", nowMs);
    return;
  }

  if (strcmp(type, "moveTo") == 0) {
    if (!doc["pan"].is<float>() || !doc["tilt"].is<float>() || !doc["zoom"].is<float>()) {
      sendError(client, "invalid_payload", "Missing target fields", nowMs);
      return;
    }
    const float pan = doc["pan"].as<float>();
    const float tilt = doc["tilt"].as<float>();
    const float zoom = doc["zoom"].as<float>();
    motion_->moveTo(pan, tilt, zoom);
    sendAck(client, "moveTo", nowMs);
    return;
  }

  if (strcmp(type, "stop") == 0) {
    motion_->stop();
    sendAck(client, "stop", nowMs);
    return;
  }

  sendError(client, "unknown_type", "Unknown command type", nowMs);
}

void PtzWebSocket::sendAck(AsyncWebSocketClient* client, const char* refType, uint32_t nowMs) {
  StaticJsonDocument<192> doc;
  doc["v"] = kProtocolVersion;
  doc["type"] = "ack";
  doc["timestampMs"] = nowMs;
  doc["refType"] = refType;

  char buffer[192];
  const size_t size = serializeJson(doc, buffer, sizeof(buffer));
  client->text(buffer, size);
}

void PtzWebSocket::sendError(AsyncWebSocketClient* client,
                             const char* code,
                             const char* message,
                             uint32_t nowMs) {
  StaticJsonDocument<256> doc;
  doc["v"] = kProtocolVersion;
  doc["type"] = "error";
  doc["timestampMs"] = nowMs;
  doc["code"] = code;
  doc["message"] = message;

  char buffer[256];
  const size_t size = serializeJson(doc, buffer, sizeof(buffer));
  client->text(buffer, size);
}

} // namespace ptz
