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







void websrvc::loop() {
}



void websrvc::loop_1s() {
}





////////////////////////////////////////////////////////////////////////////////////////////////////////
// webser stream buffer functions



uint16_t webserc::s_available() {       // we don't poll the web terminal, so if we try directly check the 'port' for data, return 0 - nothing available
  return 0;                             // the websocket functions will fill in the receive queue when the web terminal sends us data
}
char webserc::s_read() {                // we don't poll the web terminal, so any server side reads return null
  return 0;
}




void webserc::s_write(char c) {         // send a character to the web terminal
  if (!connected) return;
  web.ws->textAll(&c,1);                // send the single character to the web terminal
}



void webserc::loop() {  
  if (connected) {
    uint16_t n = qout.available();        // send any data from the queue to the web socket
    if (n > 0) {
      uint8_t buf[n];
      n = qout.read((char*)buf,n);   
      if (n > 0) web.ws->textAll(buf, n);  // Send the raw character chunk directly to the web socket instantly
    }
  }
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
