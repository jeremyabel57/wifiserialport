//#define DEBUG3
//#define DEBUG2
//#define DEBUG1





#define SERTIE_C
#include "sertie.h"


#include "config.h"
#include "general.h"      //
#include "sertie.h"       //
#include "serialq.h"      //
#include "serial2q.h"     //
#include "web.h"          //
#include "btserial.h"     //







void sertiec::begin() {

}



void sertiec::loop() {
#if WEB_SERIAL_DIAGNOSTICS
  static uint32_t lastReport = 0;
  static uint32_t webToSerial2Bytes = 0;
  static uint32_t serial2ToWebBytes = 0;
#endif

  for (uint8_t i = 0; i < 100; i++) {
    if (SERIAL2Q.available()) {   // if available data from Serial2
      char c = SERIAL2Q.read();               // read it
      SERIALQ.write(c);                       // send a copy to the usb serial
#ifdef BTSER_H
      if (btser.connected.load()) btser.write(c);  // send a copy to the Bluetooth BLE client
#endif
#ifdef WEB_H
      webser.write(c);                        // broadcast a copy to connected web terminals
#if WEB_SERIAL_DIAGNOSTICS
      serial2ToWebBytes++;
#endif
#endif
    }
    if (SERIALQ.available()) SERIAL2Q.write(SERIALQ.read());    // if traffic from Serial (usb), then send it to Serial2
#ifdef BTSER_H
    if (btser.available()) SERIAL2Q.write(btser.read());        // if traffic from BT serial, then send it to Serial2
#endif
#ifdef WEB_H
    if (webser.available()) {
      SERIAL2Q.write(webser.read());                           // if traffic from web terminal, then send it to Serial2
#if WEB_SERIAL_DIAGNOSTICS
      webToSerial2Bytes++;
#endif
    }
#endif
  }

#if WEB_SERIAL_DIAGNOSTICS
  const uint32_t now = millis();
  if (now - lastReport >= 1000) {
    Serial.printf("[WEB TRACE] web->Serial2 queued=%lu bytes/s; Serial2->web queued=%lu bytes/s\n",
                  static_cast<unsigned long>(webToSerial2Bytes),
                  static_cast<unsigned long>(serial2ToWebBytes));
    webToSerial2Bytes = 0;
    serial2ToWebBytes = 0;
    lastReport = now;
  }
#endif
}

//
