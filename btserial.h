


#ifndef BTSER_H
#define BTSER_H


/*

  btser.begin();  // setup
  btser.loop();   // service the bt serial virtual device


*/

#include <inttypes.h>       //
#include <Arduino.h>        //

#include "general.h"        // misc functions
#include "serialq.h"        // serial queue

#include <BLEDevice.h>      // bluetooth low energy libraries
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>



class btserc : public SerialQc {

  protected:

    uint16_t s_available();     // check if there is data from the bluetooth serial port
    void s_write(char);         // send a character to the bluetooth serial port
    char s_read();              // read a character from the bluetooth serial port

  public:

    BLEServer *pServer = NULL;
    BLECharacteristic *pTxCharacteristic;
    bool connected = false;
    bool oldconnected = false;
    uint8_t txValue = 0;

    void begin();               // initialize (setup)
    void loop();                // run in loop to respond to serial input

};


#ifdef BTSER_C
class btserc btser;
#else
extern class btserc btser;  // serial interface object
#endif


#endif





//
