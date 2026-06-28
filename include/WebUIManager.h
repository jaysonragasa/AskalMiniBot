#pragma once
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "Interfaces.h"

class WebUIManager {
public:
    WebUIManager(IInputReceiver& inputReceiver, IConfigRepository& configRepository);
    ~WebUIManager() = default;

    void begin();
    void loop(); // Process websocket clients

private:
    AsyncWebServer server;
    AsyncWebSocket ws;

    IInputReceiver& input;
    IConfigRepository& config;

    void handleWebSocketMessage(void *arg, uint8_t *data, size_t len, AsyncWebSocketClient *client);
};
