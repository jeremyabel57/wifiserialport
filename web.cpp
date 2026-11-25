#define WEB_C
#include "web.h"                // http server hooks to web pages

#include "serialq.h"            // queue buffer for serial output
#ifndef SERIALQ                 //
#define SERIALQ Serial          //
#endif                          //

#include "config.h"             //
#include "general.h"            // misc functions
#include <AsyncHTTPRequest_Generic.h>                           // Async web requests   https://github.com/khoih-prog/AsyncHTTPRequest_Generic





void websrvc::begin() {

  server = new AsyncFsWebServer(LittleFS, 80, "");              // AsyncFsWebServer object


  // set configuration defaults
  cfg.name = String("new");

  if (LittleFS.exists(server->getConfiFileName())) {                      // load option variables from config
    // Test "options" values
    server->getOptionValue("hostname", cfg.name);
  }

  server->setHostname(cfg.name.c_str());        // set hostname


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



void websrvc::root(AsyncWebServerRequest* request) {
  AsyncResponseStream *response = request->beginResponseStream("text/html");
  response->addHeader("Server", "ESP Async Web Server");
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
        /* Maintains spacing and inline flows */
        margin-bottom: 15px;
      }

      .input-area {
        display: flex;
        gap: 10px;
      }

      input[type="text"] {
        flex: 1;
        background-color: #1a1a1a;
        border: 1px solid #00ff00;
        color: #00ff00;
        padding: 12px;
        font-family: inherit;
        font-size: 16px;
        border-radius: 4px;
        outline: none;
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
    </div>
    <div id="terminal"></div>
    <div class="input-area">
      <input type="text" id="cmdInput" placeholder="Type directly to stream keystrokes..." autocomplete="off">
    </div>
    <script>
      const terminal = document.getElementById('terminal');
      const cmdInput = document.getElementById('cmdInput');
      const wsUrl = `ws://${window.location.host}/ws`;
      let socket;

      function logSystem(message) {
        const div = document.createElement('div');
        div.className = 'system-msg';
        div.textContent = message;
        terminal.appendChild(div);
        terminal.scrollTop = terminal.scrollHeight;
      }

      function appendChar(char) {
        // Convert newlines to block elements or linebreaks if necessary, 
        // otherwise append raw text node for perfect inline alignment
        if (char === '\r') return;
        if (char === '\n') {
          terminal.appendChild(document.createElement('br'));
        } else {
          const span = document.createTextNode(char);
          terminal.appendChild(span);
        }
        terminal.scrollTop = terminal.scrollHeight;
      }

      function initWebSocket() {
        logSystem("Connecting to ESP32 WebSocket...");
        socket = new WebSocket(wsUrl);
        socket.onopen = () => logSystem("Connection established!");
        socket.onmessage = (event) => {
          // Handle stream data character by character or word fragments instantly
          for (let i = 0; i < event.data.length; i++) {
            appendChar(event.data[i]);
          }
        };
        socket.onclose = () => {
          logSystem("Connection lost. Reconnecting in 3 seconds...");
          setTimeout(initWebSocket, 3000);
        };
      }
      // Stream every single key change non-blocking
      cmdInput.addEventListener('keydown', (e) => {
        if (!socket || socket.readyState !== WebSocket.OPEN) return;
        let payload = "";
        if (e.key.length === 1) {
          payload = e.key; // Alpha-numeric/symbols
        } else if (e.key === 'Enter') {
          payload = "\n";
        } else if (e.key === 'Backspace') {
          payload = "\b"; // Standard backspace control character
        }
        if (payload !== "") {
          socket.send(payload);
          // Clear input box immediately if user hits Enter to start a clean line block
          if (e.key === 'Enter') {
            setTimeout(() => {
              cmdInput.value = '';
            }, 10);
          }
        }
      });
      window.addEventListener('load', initWebSocket);
    </script>
  </body>
</html>
)rawliteral"));

  request->send(response);   // send response
}




void websrvc::dstore(AsyncWebServerRequest* request) {
  AsyncResponseStream *response = request->beginResponseStream("text/html");
  response->addHeader("Server","ESP Async Web Server");
  int n = request->params();                                        // number of arguments in url QUERY string
  String var, val;
  for (uint8_t i=0; i<n; i++) {
    AsyncWebParameter* p = (AsyncWebParameter*) (request->getParam(i));
    var=p->name();
    val=p->value();
    if (val != "")                                                  // if value not empty then
      web.setOpt(var.c_str(),(char*)val.c_str());                   //   set new value in storage
    val=web.getOpt(var.c_str());
    response->print(String(var + "=" + val + "\n").c_str());        // send variable and value in http respose
  }
  if (n == 0) {
    response->printf("name=%s\r\n",cfg.name);
 }
  request->send(response);                                          // close http response
}




//        get and set options / configuration

String websrvc::getOpt(const char *lbl) {                                   // get option by name
  if (strcmp(lbl,"hostname")==0)              { server->getOptionValue(lbl, cfg.name);              return cfg.name;                    }
  return String("");
}

void websrvc::setOpt(const char *lbl, char *val) {                          // set option by name
  if (strcmp(lbl,"hostname")==0)              { cfg.name = String(val);           server->saveOptionValue(lbl, cfg.name);               }
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



void websrvc::loop_1s() {
}





////////////////////////////////////////////////////////////////////////////////////////////////////////
// webser stream buffer functions



void webserc::s_write(char c) {         // send a character to the web terminal
  if (web.ws) web.ws->textAll(&c, 1);
}


bool webserc::begin() {
  SerialQc::begin(nullptr, 0, 256);
  rxQueue = xQueueCreate(1024, sizeof(uint8_t));
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
  char buf[256];
  uint16_t n = qout.read(buf, static_cast<uint16_t>(sizeof(buf)));
  if (n && web.ws) web.ws->textAll(buf, n);
}






////////////////////////////////////////////////////////////////////////////////////////////////////////
// http client functions



//        send - start a http request
bool webreq::send(const char *url) {
  if (request.readyState() == readyStateUnsent || request.readyState() == readyStateDone) {
    if (request.open("GET", url)) {
      request.send();
      processed=false;
      return true;
    }
  }
  return false;
}

bool webreq::received() { if (!processed) if (request.readyState()==readyStateDone) { processed=true; return true;}  return false; }          // true if http request complete - response received
bool webreq::pending() { return (request.readyState() != readyStateUnsent) && (request.readyState() != readyStateDone); }                     // true if http request is pending
String webreq::responseText() { return request.responseText(); }                                                                              // text of http response
char *webreq::respHeaderValue(const char *var) { return request.respHeaderValue(var); }                                                       // return value of a given header from response





//
