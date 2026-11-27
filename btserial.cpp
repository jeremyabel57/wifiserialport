


#define BTSER_C
#include "btserial.h"
#include <BLEAdvertising.h>


//  == == == == == == == == == == == == == == == == == == == == == == == == ==
//  1. ARDUINO CODE (BLE UART / NORDIC UART SERVICE)
//  == == == == == == == == == == == == == == == == == == == == == == == == ==

#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
#define DEVICE_NAME "ESP32S3_Serial"


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      btser.connected.store(true);
    };
    void onDisconnect(BLEServer* pServer) {
      btser.disconnected();
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      auto rxValue = pCharacteristic->getValue();
      if (rxValue.length() > 0) {
        size_t received = btser.receive(
                            reinterpret_cast<const uint8_t *>(rxValue.c_str()), rxValue.length());
        if (received < rxValue.length()) {
          Serial.printf("Bluetooth serial RX queue full; dropped %u byte(s)\n",
                        static_cast<unsigned>(rxValue.length() - received));
        }
      }
    }
};










void btserc::s_write(char c) {         // the program should not need to use this function, it is just here because.  send a character to the bluetooth serial port
  if (connected.load()) {
    pTxCharacteristic->setValue(c);
    pTxCharacteristic->notify();
  }
}



void btserc::begin() {
  SerialQc::begin(nullptr, 0, 256);
  rxQueue = xQueueCreate(2048, sizeof(uint8_t));
  if (!rxQueue) {
    Serial.println(F("Failed to allocate Bluetooth serial receive queue"));
    abort();
  }

  BLEDevice::init(DEVICE_NAME);
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
      BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
                                         );
  pRxCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();
  BLEAdvertising *advertising = pServer->getAdvertising();
  advertising->addServiceUUID(SERVICE_UUID);
  advertising->setScanResponse(true);
  BLEAdvertisementData scanResponse;
  scanResponse.setName(DEVICE_NAME);
  if (!advertising->setScanResponseData(scanResponse)) {
    Serial.println(F("Failed to configure Bluetooth scan response"));
  }
  advertising->start();
}


void btserc::loop() {
  if (clearQueuesPending.exchange(false)) {
    qout.clear();
  }

  if (!connected.load()) {
    if (oldconnected && pServer) pServer->startAdvertising();
    oldconnected = false;
    return;
  }
  oldconnected = true;

  uint8_t buf[20];  // Default BLE ATT MTU leaves 20 bytes for characteristic data.
  while (connected.load() && !qout.empty()) {
    uint16_t count = 0;
    while (count < sizeof(buf) && !qout.empty()) {
      buf[count++] = static_cast<uint8_t>(qout.pull());
    }
    pTxCharacteristic->setValue(buf, count);
    pTxCharacteristic->notify();
  }
}

void btserc::disconnected() {
  connected.store(false);
  if (rxQueue) xQueueReset(rxQueue);
  clearQueuesPending.store(true);
}

size_t btserc::receive(const uint8_t *data, size_t len) {
  if (!connected.load() || !rxQueue || (!data && len)) return 0;

  size_t received = 0;
  while (received < len &&
         xQueueSend(rxQueue, data + received, 0) == pdTRUE) {
    received++;
  }
  return received;
}

uint16_t btserc::available() {
  if (!connected.load() || !rxQueue) return 0;
  return static_cast<uint16_t>(uxQueueMessagesWaiting(rxQueue));
}

char btserc::read() {
  uint8_t value;
  if (!connected.load() || !rxQueue ||
      xQueueReceive(rxQueue, &value, 0) != pdTRUE) {
    return 0;
  }
  return static_cast<char>(value);
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
