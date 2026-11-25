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
  for (uint8_t i = 0; i < 100; i++) {
    if (SERIAL2Q.available()) {   // if available data from Serial2
      char c = SERIAL2Q.read();               // read it
      SERIALQ.write(c);                       // send a copy to the usb serial
#ifdef BTSER_H
      if (btser.connected.load()) btser.write(c);  // send a copy to the Bluetooth BLE client
#endif
#ifdef WEB_H
      webser.write(c);                        // broadcast a copy to connected web terminals
#endif
    }
    if (SERIALQ.available()) SERIAL2Q.write(SERIALQ.read());    // if traffic from Serial (usb), then send it to Serial2
#ifdef BTSER_H
    if (btser.available()) SERIAL2Q.write(btser.read());        // if traffic from BT serial, then send it to Serial2
#endif
#ifdef WEB_H
    if (webser.available()) SERIAL2Q.write(webser.read());      // if traffic from web terminal, then send it to Serial2
#endif
  }
}

//
