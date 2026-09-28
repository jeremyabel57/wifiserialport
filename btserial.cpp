


#define BTSER_C
#include "btserial.h"


//  == == == == == == == == == == == == == == == == == == == == == == == == ==
//  1. ARDUINO CODE (BLE UART / NORDIC UART SERVICE)
//  == == == == == == == == == == == == == == == == == == == == == == == == ==

#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      btser.connected = true;
    };
    void onDisconnect(BLEServer* pServer) {
      btser.connected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String rxValue = pCharacteristic->getValue();
      if (rxValue.length() > 0) {
        for (int i = 0; i < rxValue.length(); i++) {
          btser.write(rxValue[i]);     // push bytes received into the receive queue
        }
      }
    }
};





uint16_t btserc::s_available() {      // check if there is data from the hw bluetooth serial port - always return 0 here.  We will push characters into the queue from other functions
  return 0;
}
char btserc::s_read() {               // read a character from the bluetooth serial port, this function should not be used.  we will push characters into the queue from other functions
  return 0;                           // characters are pushed into the queue via the bt ble functions
}




void btserc::s_write(char c) {         // the program should not need to use this function, it is just here because.  send a character to the bluetooth serial port
  if (connected) {
    pTxCharacteristic->setValue(c);
    pTxCharacteristic->notify();
  }
}



void btserc::begin() {
  BLEDevice::init("ESP32S3_Serial");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pTxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_TX,
                        BLECharacteristic::PROPERTY_NOTIFY
                      );
  pTxCharacteristic->addDescriptor(new BLE2902());
  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_RX,
      BLECharacteristic::PROPERTY_WRITE
                                         );
  pRxCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();
  pServer->getAdvertising()->addServiceUUID(SERVICE_UUID);
  pServer->getAdvertising()->start();
}


void btserc::loop() {
  if (!connected && oldconnected) {
    mdelay(500);
    pServer->startAdvertising();
  }
  oldconnected = connected;

  if (connected) {
  	uint16_t chunk = qout.available();
    while (chunk > 0) {
  	  if (chunk > 20) chunk = 20; // Safe BLE MTU limit
  	
  	  uint8_t buf[chunk];
  	  chunk = qout.read((char*)buf, chunk);
  	  if (chunk > 0) {
  	    pTxCharacteristic->setValue(buf, chunk);
  	    pTxCharacteristic->notify();
      }
  	  chunk = qout.available();
    }
  }
}






/*
  == == == == == == == == == == == == == == == == == == == == == == == == ==
  2. CRITICAL IDE SETUP STEPS
  == == == == == == == == == == == == == == == == == == == == == == == == ==
  Because you are using an ESP32 - S3, verify these options under Tools in the Arduino IDE:
  Board: ESP32S3 Dev Module ( or your exact variant)
  USB CDC On Boot: Enabled (ensures Serial.print connects properly to native USB)

  == == == == == == == == == == == == == == == == == == == == == == == == ==
  3. TESTING AND CONNECTING
  == == == == == == == == == == == == == == == == == == == == == == == == ==
  Traditional Bluetooth Classic settings menus won't display BLE UART serial connections. You must use a dedicated terminal app:

  Android: Download "Serial Bluetooth Terminal" by Kai Morich
  iOS / Mac: Download "Bluefruit Connect" by Adafruit

  Steps to connect:
  1. Open the app and scan via the BLE tab.
  2. Connect to "ESP32S3_Serial".
  3. Open your Arduino IDE Serial Monitor (115200 baud).
  4. Send text back and forth between the app terminal and your PC Serial Monitor.
*/
