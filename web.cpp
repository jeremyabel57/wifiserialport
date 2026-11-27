#define WEB_C
#include "web.h"                // http server hooks to web pages

#include "serialq.h"            // queue buffer for serial output
#ifndef SERIALQ                 //
#define SERIALQ Serial          //
#endif                          //

#include "config.h"             //
#include "general.h"            // misc functions
#include "fancontroller.h"      // temperature and fan speed readings
#include <WiFi.h>
#include <math.h>


namespace {
String htmlEscape(const String& value) {
  String escaped;
  escaped.reserve(value.length());
  for (size_t i = 0; i < value.length(); i++) {
    switch (value.charAt(i)) {
      case '&': escaped += F("&amp;"); break;
      case '<': escaped += F("&lt;"); break;
      case '>': escaped += F("&gt;"); break;
      case '"': escaped += F("&quot;"); break;
      case '\'': escaped += F("&#39;"); break;
      default: escaped += value.charAt(i); break;
    }
  }
  return escaped;
}
}



void websrvc::begin() {

  server = new AsyncFsWebServer(LittleFS, 80, "");                        // AsyncFsWebServer object


  // set configuration defaults
  cfg.name = String("new");                                               // default wifi ssid name 

  if (LittleFS.exists(server->getConfiFileName())) {                      // load option variables from config
    // Test "options" values
    server->getOptionValue("hostname", cfg.name);
  }

  server->setHostname(cfg.name.c_str());                                  // set hostname


  captivePortal = !server->startWiFi(10000);                              // Try to connect to WiFi (will start AP if not connected after timeout)
  if (captivePortal) {                                                    // If not able to configure wifi connection then
    Serial.println("\r\nWiFi not connected! Starting AP mode...");        //   start captive wifi setup portal
    String t = String(cfg.name) + "_Serial";                              //   set Wifi SSID name to be devname + Setup
    server->startCaptivePortal(t.c_str(), "", "/setup");                  //   start captive setup portal
    Serial.print("AP mode, Captive Portal at SSID: ");
    Serial.println(t.c_str());
  }                                                                       // end if

  // custom options
  server->addOptionBox("Custom options");
  server->addOption("hostname", cfg.name);
  server->setSetupPageTitle("Async ESP FS WebServer");                    // set main webpage title


  //////////////////////////////////
  // our HTTP server hooks
  server->on("/led", HTTP_GET, web.handleLed);                            // Custom endpoint handler
  server->on("/resetwifi", HTTP_GET, web.clearWifi);
  server->on("/reset", HTTP_GET, web.reset);
  server->on("/ds", HTTP_GET, web.dstore);
  server->on("/status", HTTP_GET, web.status);
  server->on("/", HTTP_GET, web.root);
  //////////////////////////////////

  ws = new AsyncWebSocket("/ws");                 // configure websocket handler
  ws->onEvent(onWsEvent);
  server->addHandler(ws);

  if (!webser.begin()) {
    Serial.println(F("Failed to allocate web serial receive queue"));
    abort();
  }

  server->enableFsCodeEditor();                                           // Enable ACE FS file web editor and add FS info callback function
  server->init();                                                         // Start AsyncFsWebServer

  Serial.print(F("\r\nAsync ESP Fs Web Server started on IP Address: "));
  Serial.println(server->getServerIP());
  Serial.println(F(
                   "Open /setup page to configure optional parameters.\r\n"
                   "Open /edit page to view, edit or upload example or your custom webserver source files."));

}



void websrvc::end() {
  //LittleFS.flush();
}



void websrvc::handleLed(AsyncWebServerRequest* request) {
  static int value = false;
  // http://xxx.xxx.xxx.xxx/led?val=1
  if (request->hasParam("val")) {
    value = request->arg("val").toInt();
  } else {
    value = !digitalRead(LEDPIN);
  }
  digitalWrite(LEDPIN, value);
  String reply = "LED is now ";
  reply += (!value) ? "ON" : "OFF";
  request->send(200, "text/plain", reply);
}




void websrvc::reset(AsyncWebServerRequest* request) {
  request->send(200, "text/html",
                F(""
                  "<html><head><meta http-equiv='refresh' content='10; url=/' /></head>"
                  "<body>Rebooting Microcontroller.  Waiting 10 sec until refresh...</body></html>"));
  mdelay(1000);
  ESP.restart();  // reset mcu
}



void websrvc::clearWifi(AsyncWebServerRequest* request) {
  request->send(200, "text/html", F("<html><body><center>Wifi settings have been reset.</center></body></html>"));
  delay(500);
  WiFi.disconnect(true, true);      // first true = turn off wifi radio, second true = erase stored credentials
  delay(1000);
  ESP.restart();                    // reset mcu
}


void websrvc::status(AsyncWebServerRequest* request) {
  const float temperature = fanController.temperatureC();
  const float fanSpeed = fanController.fanSpeedPercent();
  const float fanRpm = fanController.fanRpm();

  char body[160];
  if (isfinite(temperature)) {
    snprintf(body, sizeof(body),
             "{\"temperatureC\":%.1f,\"fanSpeedPercent\":%.0f,\"fanRpm\":%.0f}",
             temperature, fanSpeed, fanRpm);
  } else {
    snprintf(body, sizeof(body),
             "{\"temperatureC\":null,\"fanSpeedPercent\":%.0f,\"fanRpm\":%.0f}",
             fanSpeed, fanRpm);
  }

  AsyncWebServerResponse *response =
    request->beginResponse(200, "application/json", body);
  response->addHeader("Cache-Control", "no-store");
  request->send(response);
}



void websrvc::root(AsyncWebServerRequest* request) {
  AsyncResponseStream *response = request->beginResponseStream("text/html");
  response->addHeader("Server", "ESP Async Web Server");

  String wifiSsid;
  IPAddress wifiIp;
  if (web.captivePortal) {
    wifiSsid = String(cfg.name) + "_Serial";
    wifiIp = WiFi.softAPIP();
  } else {
    wifiSsid = WiFi.SSID();
    wifiIp = WiFi.localIP();
  }

  response->print(F(R"rawliteral(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Real-Time Serial Terminal</title>
    <style>
      body {
        background-color: #121212;
        color: #00ff00;
        font-family: 'Courier New', Courier, monospace;
        margin: 20px;
        display: flex;
        flex-direction: column;
        height: 90vh;
      }

      h2 {
        color: #ffffff;
        margin-bottom: 5px;
      }

      .links-container {
        margin-bottom: 15px;
        display: flex;
        gap: 15px;
      }

      .custom-link {
        color: #00bfff;
        text-decoration: none;
        font-weight: bold;
        border: 1px solid #00bfff;
        padding: 5px 10px;
        border-radius: 4px;
        font-size: 14px;
        transition: all 0.2s ease;
      }

      .custom-link:hover {
        background-color: rgba(0, 191, 255, 0.2);
        box-shadow: 0 0 8px rgba(0, 191, 255, 0.5);
      }

      #terminal {
        flex: 1;
        background-color: #000000;
        border: 2px solid #333;
        border-radius: 5px;
        padding: 15px;
        overflow-y: auto;
        white-space: pre-wrap;
        margin-bottom: 15px;
      }

      .system-msg {
        color: #888888;
        font-style: italic;
        display: block;
      }
    </style>
  </head>
  <body>
    <h2> ESP32 Web Serial Terminal (Real-Time)</h2>
    <div class="links-container">
      MCU configuration: &nbsp;
      <a href="/setup" target="_blank" class="custom-link">setup</a>
      <a href="/edit" target="_blank" class="custom-link">edit</a>
)rawliteral"));
  response->print(F("&nbsp; <span>WiFi SSID: "));
  response->print(htmlEscape(wifiSsid));
  response->print(F("</span>&nbsp; <span>IP Address: "));
  response->print(wifiIp.toString());
  response->print(F("</span>&nbsp; <span>Temperature: <b id=\"temperatureStatus\">--</b></span>"
                     "&nbsp; <span>Fan: <b id=\"fanStatus\">--</b></span>"));
  response->print(F(R"rawliteral(
    </div>
    <div id="terminal"></div>
    <script>
      const terminal = document.getElementById('terminal');
      const wsProtocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
      const wsUrl = `${wsProtocol}//${window.location.host}/ws`;
      let socket;
      let decoder = new TextDecoder();
      let pendingCarriageReturn = false;
      let composing = false;

      const arrowKeys = {
        ArrowUp: 'A',
        ArrowDown: 'B',
        ArrowRight: 'C',
        ArrowLeft: 'D',
        Home: 'H',
        End: 'F'
      };
      const tildeKeys = {
        Insert: 2,
        Delete: 3,
        PageUp: 5,
        PageDown: 6
      };
      const functionKeys = {
        F1: 'P',
        F2: 'Q',
        F3: 'R',
        F4: 'S'
      };
      const numberedFunctionKeys = {
        F5: 15,
        F6: 17,
        F7: 18,
        F8: 19,
        F9: 20,
        F10: 21,
        F11: 23,
        F12: 24
      };

      function logSystem(message) {
        const div = document.createElement('div');
        div.className = 'system-msg';
        div.textContent = message;
        terminal.appendChild(div);
        terminal.scrollTop = terminal.scrollHeight;
      }

      function appendSerialText(text) {
        if (pendingCarriageReturn && text.length > 0) {
          if (text[0] === '\n') text = text.slice(1);
          pendingCarriageReturn = false;
        }
        if (!text) return;

        const endsWithCarriageReturn = text.endsWith('\r');
        if (endsWithCarriageReturn) {
          text = text.slice(0, -1);
          pendingCarriageReturn = true;
        }
        const displayText = text.replace(/\r\n/g, '\n').replace(/\r/g, '\n') +
          (endsWithCarriageReturn ? '\n' : '');
        if (!displayText) return;
        terminal.appendChild(document.createTextNode(displayText));
        terminal.scrollTop = terminal.scrollHeight;
      }

      function modifiedCsi(event, final) {
        const modifier = 1 + (event.shiftKey ? 1 : 0) +
          (event.altKey ? 2 : 0) + (event.ctrlKey ? 4 : 0);
        return modifier === 1
          ? `\x1b[${final}`
          : `\x1b[1;${modifier}${final}`;
      }

      function controlCharacter(key) {
        const normalizedKey = key.toLowerCase();
        if (normalizedKey.length === 1 &&
            normalizedKey >= 'a' && normalizedKey <= 'z') {
          return String.fromCharCode(normalizedKey.charCodeAt(0) - 96);
        }
        const controlKeys = {
          ' ': '\x00',
          '@': '\x00',
          '[': '\x1b',
          '\\': '\x1c',
          ']': '\x1d',
          '^': '\x1e',
          '_': '\x1f',
          '?': '\x7f'
        };
        return Object.prototype.hasOwnProperty.call(controlKeys, key)
          ? controlKeys[key]
          : null;
      }

      function keyPayload(event) {
        const key = event.key;
        if (event.isComposing || key === 'Process' || event.metaKey) return null;

        if (key === 'Enter') return '\r';
        if (key === 'Backspace') return event.ctrlKey ? '\x17' : '\b';
        if (key === 'Tab') return event.shiftKey ? '\x1b[Z' : '\t';
        if (key === 'Escape') return '\x1b';

        if (Object.prototype.hasOwnProperty.call(arrowKeys, key)) {
          return modifiedCsi(event, arrowKeys[key]);
        }
        if (Object.prototype.hasOwnProperty.call(tildeKeys, key)) {
          const modifier = 1 + (event.shiftKey ? 1 : 0) +
            (event.altKey ? 2 : 0) + (event.ctrlKey ? 4 : 0);
          return modifier === 1
            ? `\x1b[${tildeKeys[key]}~`
            : `\x1b[${tildeKeys[key]};${modifier}~`;
        }
        if (Object.prototype.hasOwnProperty.call(functionKeys, key)) {
          const modifier = 1 + (event.shiftKey ? 1 : 0) +
            (event.altKey ? 2 : 0) + (event.ctrlKey ? 4 : 0);
          return modifier === 1
            ? `\x1bO${functionKeys[key]}`
            : `\x1b[1;${modifier}${functionKeys[key]}`;
        }
        if (Object.prototype.hasOwnProperty.call(numberedFunctionKeys, key)) {
          const modifier = 1 + (event.shiftKey ? 1 : 0) +
            (event.altKey ? 2 : 0) + (event.ctrlKey ? 4 : 0);
          return modifier === 1
            ? `\x1b[${numberedFunctionKeys[key]}~`
            : `\x1b[${numberedFunctionKeys[key]};${modifier}~`;
        }

        const altGraph = event.getModifierState &&
          event.getModifierState('AltGraph');
        if (event.ctrlKey && !altGraph) return controlCharacter(key);
        if (key.length === 0 || Array.from(key).length !== 1) return null;
        if (altGraph) return key;
        return event.altKey ? `\x1b${key}` : key;
      }

      function sendInput(text) {
        if (!text || !socket || socket.readyState !== WebSocket.OPEN) return false;
        socket.send(text);
        return true;
      }

      async function updateStatus() {
        try {
          const response = await fetch('/status', { cache: 'no-store' });
          if (!response.ok) return;
          const status = await response.json();
          document.getElementById('temperatureStatus').textContent =
            status.temperatureC === null
              ? 'Sensor unavailable'
              : `${status.temperatureC.toFixed(1)} C`;
          document.getElementById('fanStatus').textContent =
            `${status.fanSpeedPercent.toFixed(0)}% (${status.fanRpm.toFixed(0)} RPM)`;
        } catch (_) {
          document.getElementById('temperatureStatus').textContent = 'Unavailable';
          document.getElementById('fanStatus').textContent = 'Unavailable';
        }
      }

      function initWebSocket() {
        if (socket && (socket.readyState === WebSocket.CONNECTING ||
                       socket.readyState === WebSocket.OPEN)) return;
        decoder = new TextDecoder();
        pendingCarriageReturn = false;
        logSystem("Connecting to ESP32 WebSocket...");
        socket = new WebSocket(wsUrl);
        socket.binaryType = 'arraybuffer';
        socket.onopen = () => {
          logSystem("Connection established!");
        };
        socket.onmessage = (event) => {
          const text = typeof event.data === 'string'
            ? event.data
            : decoder.decode(new Uint8Array(event.data), { stream: true });
          appendSerialText(text);
        };
        socket.onclose = () => {
          appendSerialText(decoder.decode());
          logSystem("Connection lost. Reconnecting in 3 seconds...");
          setTimeout(initWebSocket, 3000);
        };
      }

      document.addEventListener('keydown', (e) => {
        if (composing || document.hidden) return;
        const payload = keyPayload(e);
        if (payload !== null && sendInput(payload)) {
          e.preventDefault();
        }
      });

      document.addEventListener('paste', (e) => {
        const text = e.clipboardData && e.clipboardData.getData('text/plain');
        if (text && sendInput(text)) e.preventDefault();
      });

      document.addEventListener('compositionstart', () => {
        composing = true;
      });

      document.addEventListener('compositionend', (e) => {
        composing = false;
        if (e.data) sendInput(e.data);
      });

      window.addEventListener('load', () => {
        updateStatus();
        setInterval(updateStatus, 1000);
        initWebSocket();
      });
    </script>
  </body>
</html>
)rawliteral"));

  request->send(response);   // send response
}




void websrvc::dstore(AsyncWebServerRequest* request) {
  AsyncResponseStream *response = request->beginResponseStream("text/html");
  response->addHeader("Server", "ESP Async Web Server");
  int n = request->params();                                        // number of arguments in URL query string
  String var, val;
  for (uint8_t i = 0; i < n; i++) {
    const AsyncWebParameter *param = request->getParam(i);
    var = param->name();
    val = param->value();
    if (val.length() != 0) {
      web.setOpt(var.c_str(), val.c_str());
    }
    val = web.getOpt(var.c_str());
    response->printf("%s=%s\n", var.c_str(), val.c_str());
  }
  if (n == 0) {
    response->printf("name=%s\r\n", cfg.name.c_str());
  }
  request->send(response);
}




//        get and set options / configuration

String websrvc::getOpt(const char *lbl) {                                   // get option by name
  if (strcmp(lbl, "hostname") == 0) {
    server->getOptionValue(lbl, cfg.name);
    return cfg.name;
  }
  return String("");
}

void websrvc::setOpt(const char *lbl, const char *val) {                    // set option by name
  if (strcmp(lbl, "hostname") == 0) {
    cfg.name = String(val);
    server->saveOptionValue(lbl, cfg.name);
  }
}
  
////




// WebSocket event handler for incoming data from the browser
void websrvc::onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
               void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_DATA) {
        AwsFrameInfo *info = static_cast<AwsFrameInfo *>(arg);
        if (info && info->message_opcode == WS_TEXT) {
            size_t received = webser.receive(data, len);
            if (received < len) {
                Serial.printf("Web serial RX queue full; dropped %u byte(s)\n",
                              static_cast<unsigned>(len - received));
            }
        }
    }
}



void websrvc::loop() {
}



////////////////////////////////////////////////////////////////////////////////////////////////////////
// webser stream buffer functions



void webserc::s_write(char c) {         // send a character to the web terminal
  if (web.ws) web.ws->binaryAll(reinterpret_cast<const uint8_t *>(&c), 1);
}


bool webserc::begin() {
  SerialQc::begin(nullptr, 0, 256);
  rxQueue = xQueueCreate(2048, sizeof(uint8_t));
  return rxQueue != nullptr;
}

size_t webserc::receive(const uint8_t *data, size_t len) {
  if (!rxQueue || (!data && len)) return 0;

  size_t received = 0;
  while (received < len &&
         xQueueSend(rxQueue, data + received, 0) == pdTRUE) {
    received++;
  }
  return received;
}

uint16_t webserc::available() {
  return rxQueue ? static_cast<uint16_t>(uxQueueMessagesWaiting(rxQueue)) : 0;
}

char webserc::read() {
  uint8_t value;
  if (!rxQueue || xQueueReceive(rxQueue, &value, 0) != pdTRUE) return 0;
  return static_cast<char>(value);
}


void webserc::loop() {
  char output[256];
  uint16_t length = qout.read(output, static_cast<uint16_t>(sizeof(output)));
  if (length && web.ws) {
    web.ws->binaryAll(reinterpret_cast<const uint8_t *>(output), length);
  }
}
