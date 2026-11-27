#line 1 "D:\\Users\\person\\Documents\\vs\\wifiserialport\\btserial.h"



#ifndef BTSER_H
#define BTSER_H


/*

  btser.begin();  // setup
  btser.loop();   // service the bt serial virtual device


*/

#include <inttypes.h>       //
#include <Arduino.h>        //
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "general.h"        // misc functions
#include "serialq.h"        // serial queue

#include <BLEDevice.h>      // bluetooth low energy libraries
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>



class btserc : public SerialQc {

  protected:

    void s_write(char);         // send a character to the bluetooth serial port
    QueueHandle_t rxQueue = nullptr;
    std::atomic<bool> clearQueuesPending{false};

  public:

    BLEServer *pServer = NULL;
    BLECharacteristic *pTxCharacteristic = nullptr;
    std::atomic<bool> connected{false};
    bool oldconnected = false;

    void begin();               // initialize (setup)
    void loop();                // run in loop to respond to serial input
    void disconnected();
    size_t receive(const uint8_t *data, size_t len);
    uint16_t available();
    char read();

};


#ifdef BTSER_C
class btserc btser;
#else
extern class btserc btser;  // serial interface object
#endif


#endif





//
