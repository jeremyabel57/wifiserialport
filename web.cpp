

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
    <h2>📟 ESP32 Web Serial Terminal (Real-Time)</h2>
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
        socket.onopen = () => logSystem("⚡ Connection established!");
        socket.onmessage = (event) => {
          // Handle stream data character by character or word fragments instantly
          for (let i = 0; i < event.data.length; i++) {
            appendChar(event.data[i]);
          }
        };
        socket.onclose = () => {
          logSystem(" Connection lost. Reconnecting in 3 seconds...");
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
        AwsFrameInfo *info = (AwsFrameInfo*)arg;
        if (info->opcode == WS_TEXT) {
            // Write chunks/characters directly to the serial monitor without waiting for full lines
            webser.write(data, len);
        }
    }
}

