#pragma once
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "Interfaces.h"

/**
 * @class WebUIManager
 * @brief Manages the WiFi access point, HTTP server, and WebSocket connections.
 * 
 * Serves the HTML/JS frontend to the client and translates incoming WebSocket 
 * JSON commands into native C++ calls to the Kinematics and Config systems.
 */
class WebUIManager {
public:
    /**
     * @brief Constructs a new Web UI Manager.
     * @param inputReceiver Reference to the object handling joystick inputs.
     * @param configRepository Reference to the configuration storage.
     */
    WebUIManager(IInputReceiver& inputReceiver, IConfigRepository& configRepository);

    /**
     * @brief Destroys the Web UI Manager.
     */
    ~WebUIManager() = default;

    /**
     * @brief Starts the web server and websocket listeners.
     */
    void begin();
    
    /**
     * @brief Processes pending WebSocket client disconnections or tasks.
     * Must be called periodically in the main loop.
     */
    void loop(); 

private:
    AsyncWebServer server;
    AsyncWebSocket ws;

    IInputReceiver& input;
    IConfigRepository& config;

    /**
     * @brief Callback for incoming WebSocket messages from the browser.
     */
    void handleWebSocketMessage(void *arg, uint8_t *data, size_t len, AsyncWebSocketClient *client);
};
