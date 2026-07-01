/**
 * @file WebUIManager.cpp
 * @brief Web UI implementation: serves the single-page controller and routes
 *        incoming WebSocket JSON commands to the kinematics and config layers.
 *
 * The entire browser frontend (HTML/CSS/JS) is embedded as a PROGMEM raw string
 * literal (index_html) and served from flash. Control happens over a WebSocket
 * at "/ws"; see handleWebSocketMessage for the supported message types.
 */

#include "WebUIManager.h"
#include <ArduinoJson.h>

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>AskalMiniBot Controller</title>
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
  <style>
    body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background-color: #121212; color: #fff; margin: 0; padding: 0; overflow: hidden; user-select: none; -webkit-user-select: none; }
    
    /* Top Centered UI */
    .top-center-ui { display: flex; flex-direction: column; align-items: center; padding-top: 15px; z-index: 10; position: relative; }
    .main-title { font-size: 24px; font-weight: bold; margin-bottom: 5px; }
    #status { font-size: 12px; color: #ff4444; margin-bottom: 15px; text-transform: uppercase; letter-spacing: 1px; }
    #fs-btn { position: absolute; top: 15px; right: 15px; background: none; border: none; color: #aaa; font-size: 28px; cursor: pointer; transition: color 0.2s; padding: 5px; }
    #fs-btn:hover { color: #00ffcc; }
    
    /* Floating Tabs */
    .floating-tabs { display: flex; gap: 15px; margin-bottom: 10px; }
    .floating-tab { background: #222; color: #fff; border: 2px solid #444; padding: 10px 25px; border-radius: 20px; cursor: pointer; font-weight: bold; transition: all 0.2s; font-size: 14px; }
    .floating-tab.active { background: #00ffcc; color: #000; border-color: #00ffcc; box-shadow: 0 0 10px rgba(0, 255, 204, 0.5); }
    
    .content { padding: 10px; display: none; height: calc(100vh - 130px); overflow-y: auto; box-sizing: border-box; }
    .content.active { display: block; }
    #control.active { display: block; overflow: hidden; padding: 0; position: relative; }
    
    .joystick-container { position: absolute; top: 0; left: 0; width: 100%; height: 100%; display: flex; justify-content: space-between; align-items: center; padding: 0 5%; box-sizing: border-box; z-index: 100; pointer-events: none; }
    #left-zone, #right-zone { width: 200px; height: 200px; background: rgba(255, 255, 255, 0.05); border-radius: 50%; border: 2px solid #444; position: relative; touch-action: none; pointer-events: auto; }
    
    .center-controls { position: absolute; top: 0; left: 0; width: 100%; height: 100%; display: flex; flex-direction: column; justify-content: center; align-items: center; z-index: 150; pointer-events: none; }
    
    .form-group { margin-bottom: 25px; }
    label { display: block; margin-bottom: 8px; color: #aaa; font-size: 14px; }
    input[type="number"] { width: 100%; padding: 12px; background: #222; border: 1px solid #444; color: #fff; border-radius: 5px; box-sizing: border-box; font-size: 16px; }
    input[type="range"] { width: 100%; height: 25px; margin: 10px 0; }
    input[type="checkbox"] { margin-right: 10px; width: 20px; height: 20px; vertical-align: middle; }
    .checkbox-label { display: inline-flex; align-items: center; font-size: 16px; color: #eee; }
    .mode-row { display: flex; gap: 10px; justify-content: center; flex-wrap: wrap; width: 100%; }
    .mode-btn { pointer-events: auto; background: #222; color: #fff; border: 2px solid #333; width: 60px; height: 60px; display: flex; justify-content: center; align-items: center; cursor: pointer; border-radius: 12px; font-weight: bold; font-size: 13px; padding: 0; }
    .mode-btn.active { background: #00ffcc; color: #000; border-color: #00ffcc; box-shadow: 0 0 10px rgba(0, 255, 204, 0.5); }
    
    h3 { margin-top: 20px; border-bottom: 1px solid #444; padding-bottom: 5px; }
    
    /* Accordion styles */
    .accordion { background-color: #2a2a2a; color: #fff; cursor: pointer; padding: 15px; width: 100%; border: none; text-align: left; outline: none; font-size: 16px; font-weight: bold; border-radius: 8px; margin-bottom: 5px; transition: 0.2s; display: flex; justify-content: space-between; align-items: center; }
    .accordion.active, .accordion:hover { background-color: #3a3a3a; }
    .accordion::after { content: '\002B'; color: #00ffcc; font-weight: bold; font-size: 20px; }
    .accordion.active { border-radius: 8px 8px 0 0; }
    .accordion.active::after { content: "\2212"; }
    .panel { padding: 0 15px; background-color: #1a1a1a; max-height: 0; overflow: hidden; transition: max-height 0.2s ease-out; border-radius: 0 0 8px 8px; margin-bottom: 10px; }
    .panel-inner { padding: 15px 0; }
  </style>
  <script src="https://cdnjs.cloudflare.com/ajax/libs/nipplejs/0.10.1/nipplejs.min.js"></script>
</head>
<body>
  <div class="top-center-ui">
    <button id="fs-btn" onclick="toggleFullScreen()" title="Toggle Fullscreen">&#9974;</button>
    <div class="main-title">AskalMiniBot</div>
    <div id="status">Disconnected</div>
    <div class="floating-tabs">
      <button class="floating-tab active" onclick="switchTab('control', this)">Control</button>
      <button class="floating-tab" onclick="switchTab('settings', this)">Settings</button>
    </div>
  </div>

  <div id="control" class="content active">
    <!-- Joysticks in the middle -->
    <div class="joystick-container">
      <div id="left-zone"></div>
      <div id="right-zone"></div>
    </div>
    
    <!-- All movement buttons clustered in the dead center between joysticks -->
    <div class="center-controls">
      <div class="mode-row">
        <button class="mode-btn active" onclick="setMode(0, this)">Trot</button>
        <button class="mode-btn" onclick="setMode(1, this)">Walk</button>
        <button class="mode-btn" onclick="setMode(2, this)">Gallop</button>
        <button class="mode-btn" onclick="setMode(3, this)">Auto</button>
      </div>
      <div class="mode-row" style="margin-top: 15px;">
        <button class="mode-btn" onclick="setMode(4, this)">Sit</button>
        <button class="mode-btn" onclick="setMode(5, this)">Stretch</button>
        <button class="mode-btn" onclick="setMode(6, this)">Fold</button>
        <button class="mode-btn" onclick="setMode(7, this)">Wave</button>
      </div>
      <div class="mode-row" style="margin-top: 15px;">
        <button class="mode-btn" onclick="setMode(8, this)">Pee</button>
        <button class="mode-btn" onclick="setMode(9, this)">Scrape</button>
        <button class="mode-btn" onclick="setMode(10, this)">Info</button>
      </div>
    </div>
  </div>

  <div id="settings" class="content">
    <div id="config-form">
      <h3>Servo Configuration</h3>
      <script>
        const servoNames = ["Front-Left (0)", "Front-Right (1)", "Hind-Left (2)", "Hind-Right (3)"];
        for (let i = 0; i < 4; i++) {
          document.write(`
              <button class="accordion">${servoNames[i]}</button>
              <div class="panel">
                <div class="panel-inner">
                  <div class="form-group"><label>Pin:</label><input type="number" id="s${i}_pin" onchange="saveConfig()" /></div>
                  <div class="form-group">
                    <label>Offset: <span id="s${i}_off_val">0</span>&deg;</label>
                    <input type="range" id="s${i}_off" min="-90" max="90" value="0" oninput="updateVal(${i}); saveConfig()" />
                  </div>
                  <div class="form-group"><label class="checkbox-label"><input type="checkbox" id="s${i}_inv" onchange="saveConfig()" /> Invert Direction</label></div>
                </div>
              </div>
            `);
          }
          function updateVal(i) {
            document.getElementById(`s${i}_off_val`).innerText = document.getElementById(`s${i}_off`).value;
          }
        </script>
      </div>
      
    <div id="weather-form" style="margin-top:20px;">
      <h3>Integrations</h3>
      <button class="accordion">OpenWeather API</button>
      <div class="panel">
        <div class="panel-inner">
          <div class="form-group"><label>API Key:</label><input type="text" id="ow_key" onchange="saveWeatherConfig()" style="width:100%; padding:12px; background:#222; border:1px solid #444; color:#fff; border-radius:5px; box-sizing:border-box; font-size:16px;" /></div>
          <div class="form-group"><label>Latitude:</label><input type="number" id="ow_lat" step="any" onchange="saveWeatherConfig()" /></div>
          <div class="form-group"><label>Longitude:</label><input type="number" id="ow_lon" step="any" onchange="saveWeatherConfig()" /></div>
          <button style="width: 100%; padding: 12px; background: #00ffcc; color: #000; font-weight: bold; border: none; border-radius: 5px; cursor: pointer; font-size: 16px;" onclick="detectLocation()">Detect Location</button>
        </div>
      </div>
    </div>
  </div>

  <script>
    let ws;
    let joys = { throttle: 0, yaw: 0, pitch: 0, roll: 0 };
    let lastJoys = { throttle: 0, yaw: 0, pitch: 0, roll: 0 };
    let idleSent = false;

    function initWebSocket() {
      ws = new WebSocket(`ws://${window.location.hostname}/ws`);
      ws.onopen = () => { document.getElementById('status').innerText = 'Connected'; document.getElementById('status').style.color = '#00ffcc'; getConfig(); };
      ws.onclose = () => { document.getElementById('status').innerText = 'Disconnected'; document.getElementById('status').style.color = '#ff4444'; setTimeout(initWebSocket, 2000); };
      ws.onmessage = (event) => {
        let data = JSON.parse(event.data);
        if (data.type === 'config') {
          for(let i=0; i<4; i++) {
            document.getElementById(`s${i}_pin`).value = data.servos[i].pin;
            document.getElementById(`s${i}_off`).value = data.servos[i].offset;
            document.getElementById(`s${i}_off_val`).innerText = data.servos[i].offset;
            document.getElementById(`s${i}_inv`).checked = data.servos[i].invert;
          }
          document.getElementById('ow_key').value = data.ow_key || '';
          document.getElementById('ow_lat').value = data.ow_lat || 0;
          document.getElementById('ow_lon').value = data.ow_lon || 0;
        }
      };
    }

    function switchTab(tabId, btn) {
      document.querySelectorAll('.floating-tab').forEach(t => t.classList.remove('active'));
      document.querySelectorAll('.content').forEach(c => c.classList.remove('active'));
      if(btn) btn.classList.add('active');
      document.getElementById(tabId).classList.add('active');
    }

    function toggleFullScreen() {
      if (!document.fullscreenElement) {
        document.documentElement.requestFullscreen().catch(err => {
          console.log(`Error attempting to enable fullscreen: ${err.message}`);
        });
      } else {
        if (document.exitFullscreen) {
          document.exitFullscreen();
        }
      }
    }

    // Init Joysticks
    let optionsLeft = { zone: document.getElementById('left-zone'), mode: 'static', position: { left: '50%', top: '50%' }, color: '#00ffcc', size: 200 };
    let optionsRight = { zone: document.getElementById('right-zone'), mode: 'static', position: { left: '50%', top: '50%' }, color: '#00ffcc', size: 200 };
    let managerLeft = nipplejs.create(optionsLeft);
    let managerRight = nipplejs.create(optionsRight);

    managerLeft.on('move', (evt, data) => { 
      if (data && data.vector) {
        joys.yaw = -data.vector.x * 100; // Inverted
        joys.throttle = data.vector.y * 100; 
      } else if (data && data.angle && data.distance != null) {
        joys.yaw = -Math.cos(data.angle.radian) * (data.distance / 50) * 100; // Inverted
        joys.throttle = Math.sin(data.angle.radian) * (data.distance / 50) * 100; 
      }
      idleSent = false;
    });
    managerLeft.on('end', () => { joys.yaw = 0; joys.throttle = 0; });
    managerRight.on('move', (evt, data) => { 
      if (data && data.vector) {
        joys.roll = data.vector.x * 100;
        joys.pitch = data.vector.y * 100;
      } else if (data && data.angle && data.distance != null) {
        joys.roll = Math.cos(data.angle.radian) * (data.distance / 50) * 100; 
        joys.pitch = Math.sin(data.angle.radian) * (data.distance / 50) * 100; 
      }
      idleSent = false;
    });
    managerRight.on('end', () => { joys.roll = 0; joys.pitch = 0; });

    setInterval(() => {
      if (ws && ws.readyState === WebSocket.OPEN) {
        let isIdle = joys.throttle === 0 && joys.yaw === 0 && joys.pitch === 0 && joys.roll === 0;
        
        // Only send if we are NOT idle, OR if we just became idle (need to send zeros once)
        if (!isIdle || !idleSent) {
          ws.send(JSON.stringify({ type: 'joystick', t: joys.throttle, y: joys.yaw, p: joys.pitch, r: joys.roll }));
          if (isIdle) idleSent = true;
        }
      }
    }, 50);

    function getConfig() { ws.send(JSON.stringify({ type: 'get_config' })); }
    
    function setMode(idx, btn) {
      document.querySelectorAll('.mode-btn').forEach(b => b.classList.remove('active'));
      btn.classList.add('active');
      ws.send(JSON.stringify({ type: 'set_mode', mode: idx }));
    }

    function saveConfig() {
      let cfg = { type: 'set_config', servos: [] };
      for(let i=0; i<4; i++) {
        let pinVal = document.getElementById(`s${i}_pin`).value;
        let offVal = document.getElementById(`s${i}_off`).value;
        if(pinVal === "") pinVal = 0;
        cfg.servos.push({
          pin: parseInt(pinVal),
          offset: parseFloat(offVal),
          invert: document.getElementById(`s${i}_inv`).checked
        });
      }
      ws.send(JSON.stringify(cfg));
    }

    async function detectLocation() {
      try {
        const response = await fetch('http://ip-api.com/json/');
        const data = await response.json();
        if (data && data.status === 'success') {
          document.getElementById('ow_lat').value = data.lat;
          document.getElementById('ow_lon').value = data.lon;
          saveWeatherConfig();
          alert("Location detected: " + data.city + ", " + data.country);
        } else {
          alert("Failed to detect location from IP.");
        }
      } catch (err) {
        alert("Error fetching location: " + err.message);
      }
    }

    function saveWeatherConfig() {
      let cfg = {
        type: 'set_weather_cfg',
        ow_key: document.getElementById('ow_key').value,
        ow_lat: parseFloat(document.getElementById('ow_lat').value) || 0.0,
        ow_lon: parseFloat(document.getElementById('ow_lon').value) || 0.0
      };
      ws.send(JSON.stringify(cfg));
    }
    
    // Accordion Logic
    function setupAccordions() {
      var acc = document.getElementsByClassName("accordion");
      for (let i = 0; i < acc.length; i++) {
        acc[i].addEventListener("click", function() {
          this.classList.toggle("active");
          var panel = this.nextElementSibling;
          if (panel.style.maxHeight) {
            panel.style.maxHeight = null;
          } else {
            panel.style.maxHeight = panel.scrollHeight + "px";
          } 
        });
      }
    }
    
    // Call this after DOM loads
    setTimeout(setupAccordions, 100);

    initWebSocket();
  </script>
</body>
</html>
)rawliteral";

WebUIManager::WebUIManager(IInputReceiver& inputReceiver, IConfigRepository& configRepository)
    : server(80), ws("/ws"), input(inputReceiver), config(configRepository) {}

// -------------------------------------------------------------------------
// WebUIManager::begin
// Initializes the AsyncWebServer, configures WebSocket handlers, and serves the UI.
// -------------------------------------------------------------------------
void WebUIManager::begin() {
    // -------------------------------------------------------------------------
    // Set up WebSocket event handler
    // We only care about WS_EVT_DATA (incoming messages from the browser).
    // -------------------------------------------------------------------------
    ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
        if (type == WS_EVT_DATA) {
            this->handleWebSocketMessage(arg, data, len, client);
        }
    });
    
    // Attach websocket to the async server
    server.addHandler(&ws);

    // -------------------------------------------------------------------------
    // Serve the main UI
    // Renders the HTML string stored in PROGMEM when someone navigates to "/"
    // -------------------------------------------------------------------------
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", index_html);
    });

    server.begin();
}

// -------------------------------------------------------------------------
// WebUIManager::loop
// Must be called in the main loop to clean up disconnected WebSocket clients.
// -------------------------------------------------------------------------
void WebUIManager::loop() {
    // Disconnect dead clients and free memory. Needs to run frequently.
    ws.cleanupClients();
}

// -------------------------------------------------------------------------
// WebUIManager::handleWebSocketMessage
// Parses incoming JSON from the browser and routes it to the correct handler.
// -------------------------------------------------------------------------
void WebUIManager::handleWebSocketMessage(void *arg, uint8_t *data, size_t len, AsyncWebSocketClient *client) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    
    // Check that we received a complete text frame
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        // Safely copy data into a String to guarantee null-termination and prevent zero-copy mutation
        String jsonPayload;
        jsonPayload.reserve(len);
        for (size_t i = 0; i < len; i++) {
            jsonPayload += (char)data[i];
        }
        
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, jsonPayload);
        if (error) {
            Serial.printf("JSON parse failed: %s\n", error.c_str());
            return; // Ignore malformed JSON
        }

        const char* type = doc["type"];
        
        // ---------------------------------------------------------------------
        // INCOMING ROUTING
        // ---------------------------------------------------------------------
        if (strcmp(type, "joystick") == 0) {
            // Virtual joystick update
            float t = doc["t"]; // throttle
            float y = doc["y"]; // yaw
            float p = doc["p"]; // pitch
            float r = doc["r"]; // roll
            input.onJoystickUpdate(t, y, p, r);
            
        } else if (strcmp(type, "get_config") == 0) {
            // Client requested current settings (happens on page load)
            JsonDocument outDoc;
            outDoc["type"] = "config";
            JsonArray servos = outDoc["servos"].to<JsonArray>();
            for (int i = 0; i < 4; i++) {
                ServoConfig cfg = config.getServoConfig(i);
                JsonObject s = servos.add<JsonObject>();
                s["pin"] = cfg.pin;
                s["offset"] = cfg.offset;
                s["invert"] = (cfg.invert == -1); // true if inverted
            }
            outDoc["ow_key"] = config.getOpenWeatherKey();
            outDoc["ow_lat"] = config.getLatitude();
            outDoc["ow_lon"] = config.getLongitude();
            
            String output;
            serializeJson(outDoc, output);
            client->text(output);
            
        } else if (strcmp(type, "set_config") == 0) {
            // User saved servo calibration settings in the UI
            JsonArray servos = doc["servos"];
            for (int i = 0; i < 4; i++) {
                ServoConfig cfg = config.getServoConfig(i); // preserve other fields like maxAngle
                cfg.pin = servos[i]["pin"];
                cfg.offset = servos[i]["offset"];
                cfg.invert = servos[i]["invert"] ? -1 : 1;
                config.setServoConfig(i, cfg);
            }
            // Notify the kinematics system that calibration changed
            input.onConfigUpdated();
            
        } else if (strcmp(type, "set_mode") == 0) {
            // User clicked a gait/pose button
            int modeIdx = doc["mode"];
            input.setGait(modeIdx);
            
        } else if (strcmp(type, "set_weather_cfg") == 0) {
            // User saved weather API settings
            config.setOpenWeatherKey(doc["ow_key"]);
            config.setLatitude(doc["ow_lat"]);
            config.setLongitude(doc["ow_lon"]);
        }
    }
}
