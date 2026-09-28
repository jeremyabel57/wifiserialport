/*
next steps:
add neopixel and then update led on/off web routine to use neopixel to turn it off or on (all GRB at full brightness)

validate async web server setup and edit features - upload edit.tar.gz file for edit feature
validate serial2, serial, btser, webser connectivity
 - serial to serial2 passthru
 - btser to serial2 passthru
 - webser to serial2 passthru
also, make sure serial to serial2 passthru works when wifi is not configured yet
hopefully btserial works independent of wifi being configured


---------


goal:
serial passthru from Serial1 to Serial(USB serial)
bluetooth serial access to Serial1
- look for password on the bluetooth
wifi access to serial1
  - have password on the wifi

add an option to disable wifi and bt ble.  the option should be accessible from the usb serial port - maybe have a command interface that responds to a special pair of keystrokes, like the ssh escape sequence (\n~...)
add a couple of small commands in the sertie class responding to the escape sequence - enable/disable bt/wifi.
or possibly just have it ask for a password when bt initially connecting, and the same for web

*/

/*
   init reorder
   serial.begin(115200)
   initialize AsyncFsWebServer
   get device name from options
   set hostname in AsyncFsWebServer using device name
   finish initializing AsyncFsWebServer

   get device program type (outdoor thermometer, grandfather clock, network host) from datastore
   initialize and execute accordingly
      outdoor thermometer will have multiple instances, each instance reports its name and the data to the network host
      one outdoor thermometer will upload its data to weather underground, use a special project name to identify this one (i.e. outdoor thermon 1)
      grandfather clock reports its name, current date/time stamp, and temperature/enviornment data to the network host
   network host
      will also support distributing OTA to each client by continusouly ping them until they wake up and come online, then quickly upload OTA
        if necessary, first disable client sleep mode, then OTA update, then reenable sleep
      when receiving data, add date/time stamp to the data so when reported we can see the last update.
      sync current date time stamp from GF clock.  if not available, then sync from internet

*/




#include <Arduino.h>            // Arduino, including String
#include "serialq.h"            // queue buffer for serial i/o
#include "serial2q.h"           // queue buffered version of Serial2
#ifndef SERIALQ                 //
#define SERIALQ Serial          //
#define SERIALQ2 Serial2        //
#endif                          //


#include "general.h"            // misc functions
#include "config.h"
#include "sertie.h"             // serial tie between three clients (bt serial, web serial, and usb serial) and serial2 (the computer console)
                                //   traffice from serial2 is passed along to all three clients, input from any client is passed on to serial2
#include "btserial.h"           // bluetooth serial
#include "web.h"                // WiFi web setup, FS, HTTP server, including a web page text terminal interface for the serial port







// timers for loop function
unsigned int loopTimer;
unsigned int loopTimer1s;






void setup() {
  char *s;

  SerialQ_begin();                                                                              // initialize Serial port - main USB serial
  Serial2Q_begin();                                                                             // initialize Serial2 port - port connected to target router/computer console
  if (!LittleFS.begin(true)) Serial.println("An error has occurred while mounting LittleFS");   // Initialize LittleFS

#ifdef BTSER_H
  btser.begin();                                                                                // initialize BLE Bluetooth Low Energy serial port object
#endif

#ifdef WEB_H                                                                                    // initialize the web server
  web.begin();                                                                                  // among other things, cfg is loaded from here
#endif

  pinMode(LEDPIN, OUTPUT);                                                                      // initialize the LED pin
  digitalWrite(LEDPIN, !web.captivePortal);                                                     // set LED to show status (off = wifi configured, on = captive portal)
  
#ifdef SERTIE_H
  sertie.begin();                                                                               // initialize the code that directs serial traffic between BT serial, web serial, USB serial and Serial2
#endif

  loopTimer1s = millis();
}







void loop() {
#ifdef SERIALQ_H
  SerialQ.loop();               // serial Q handler for Serial
  Serial2Q.loop();              // serial Q handler for Serial2
#endif
#ifdef BTSER_H
  btser.loop();                 // bluetooth serial object
#endif
#ifdef SERTIE_H
  sertie.loop();                // serial tie together handler
#endif
#ifdef WEB_H
    web.loop();
#endif
 
  loopTimer = millis();

  // execute once per second
  if (elapsems(loopTimer1s, loopTimer) > 1000) {    // execute once per second
    loopTimer1s = millis();

#ifdef WEB_H
    web.loop_1s();
#endif
  }


  delay(10);    // yield to RTOS and background processes
}

//
