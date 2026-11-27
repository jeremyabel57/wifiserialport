#ifndef WEB_H
#define WEB_H


/*
*/


/*

  web.begin();    // setup including AsyncFsWebServer

*/



#include <Arduino.h>                            // arduino
#include "serialq.h"                            // queue buffer for serial stream
#include <string.h>                             // strings
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include <FS.h>                                 // AsyncFsWebServer (in place of ESP Essentials)
#include <LittleFS.h>                           // LittleFS
#include <AsyncFsWebServer.h>                   // AsyncFsWebServer (in place of ESP Essentials), this uses Async background operations
// serial queue buffer for web serial stream
class webserc : public SerialQc {
  protected:

    QueueHandle_t rxQueue = nullptr;

    void s_write(char);         // send a character to the web terminal

  public:

    bool begin();
    size_t receive(const uint8_t *data, size_t len);
    uint16_t available();
    char read();

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
    static void status(AsyncWebServerRequest*);                       // return live fan and sensor status

    static void onWsEvent(AsyncWebSocket*, AsyncWebSocketClient*, AwsEventType, void*, uint8_t*, size_t);   // websocket handler



  public:

    AsyncWebSocket *ws;                                               // AsyncWebSocket object pointer, public so that sertie can send text to it

    // FS, Server functions

    bool captivePortal;                                               // true if wifi setup captive portal active because wifi is not connecting or not configured

    void begin();                                                     // initialize (setup)
    void end();                                                       // flush LittleFS

    String getOpt(const char *lbl);                                   // get option by name
    void setOpt(const char *lbl, const char *val);                    // set option by name

    void loop();                                                      // run every loop

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
