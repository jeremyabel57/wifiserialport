#ifndef WEB_H
#define WEB_H


/*
   Todo:
   . add web page to set wakeme and monitor wakeme variable.  wakeme="" for inert.  wakeme=hostname of thermo or rtc host to keep awake.  the next time the host uploads data, it is told to toggle sleep_en, and wakeme is erased.
   . validate time sync
   . validate /dstore web interface
   . validate web pages
   . test host mode data storage and reporting /tdata.html file / web page report
*/


/*

  web.begin();    // setup including AsyncFsWebServer
  web.loop_1s();  // run every 1 second inside loop

*/



#include <Arduino.h>                            // arduino
#include "serialq.h"                            // queue buffer for serial stream
#include <string.h>                             // strings

#include <FS.h>                                 // AsyncFsWebServer (in place of ESP Essentials)
#include <LittleFS.h>                           // LittleFS
#include <AsyncFsWebServer.h>                   // AsyncFsWebServer (in place of ESP Essentials), this uses Async background operations
#include <AsyncHTTPRequest_Generic.h>           // Async web requests   https://github.com/khoih-prog/AsyncHTTPRequest_Generic



// client functions
class webreq {
  public:

    AsyncHTTPRequest request;                         // async request object
    bool processed = true;                            // use this to flag that even though readyStateDone, it has been processed, and ready for next request

    bool send(const char*);                           // send http request.  return true if successful
    bool received();                                  // returns true if response received from request.  Only returns true once, then sets processed flag so subsequent calls do not return true until next request is made.
    bool pending();                                   // returns true if web request in process
    String responseText();                            // return response text (if available)
    char *respHeaderValue(const char *);              // return given header value
} ;



// serial queue buffer for web serial stream
class webserc : public SerialQc {
 protected:

    uint16_t s_available();     // check if there is data from the web terminal
    void s_write(char);         // send a character to the web terminal
    char s_read();              // read a character from the web terminal

  public:

    bool connected = false;     // is web terminal connected / online

    void loop();                // run in loop to respond to serial input
} ;



// server and AsyncFsWebServer features
class websrvc {
  protected:

    AsyncFsWebServer *server;                                         // AsyncFsWebServer object pointer

    // web pages
    static void reset(AsyncWebServerRequest*);                        // reset mcu
    static void clearWifi(AsyncWebServerRequest*);                    // clear Wifi settings and reset mcu
    static void root(AsyncWebServerRequest*);                         // root web page
    static void dstore(AsyncWebServerRequest*);                       // handle web option requests
    static void handleLed(AsyncWebServerRequest*);                    // switch LED on/off

    static void onWsEvent(AsyncWebSocket*, AsyncWebSocketClient*, AwsEventType, void*, uint8_t*, size_t);   // websocket handler



  public:

    AsyncWebSocket *ws;                                               // AsyncWebSocket object pointer, public so that sertie can send text to it

    // FS, Server functions

    bool captivePortal;                                               // true if wifi setup captive portal active because wifi is not connecting or not configured

    void begin();                                                     // initialize (setup)
    void end();                                                       // flush LittleFS

    String getOpt(const char *lbl);                                   // get option by name
    void setOpt(const char *lbl, char *val);                          // set option by name

    void loop();                                                      // run every loop
    void loop_1s();                                                   // run every second

} ;





#ifdef WEB_C
class websrvc web;
class webserc webser;
#else
extern class websrvc web;
extern class webserc webser;
#endif


#endif

//
