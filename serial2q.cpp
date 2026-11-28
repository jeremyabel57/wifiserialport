
#define SERIAL2Q_C
#include "serial2q.h"
#include "config.h"


/*
  void Serial2Qc::begin(Stream *s1, uint16_t bin, uint16_t bout) {
  SerialQc::begin(s1, bin, bout);       // initialize the base/inherited class items
  s1->begin(115200, SERIAL_8N1, SERIAL2_RX2_PIN, SERIAL2_TX2_PIN);      // UART connected to the computer console, target of the pass thru
  }
*/


class Serial2Qc Serial2Q;

#if WEB_SERIAL_DIAGNOSTICS
namespace {
uint32_t serial2TxBytes = 0;
}
#endif

void Serial2Qc::s_write(char c) {
  SerialQc::s_write(c);
#if WEB_SERIAL_DIAGNOSTICS
  serial2TxBytes++;
#endif
}

void Serial2Qc::loop() {
  SerialQc::loop();
#if WEB_SERIAL_DIAGNOSTICS
  static uint32_t lastReport = 0;
  const uint32_t now = millis();
  if (now - lastReport >= 1000) {
    Serial.printf("[SERIAL2 TRACE] UART write calls=%lu bytes/s\n",
                  static_cast<unsigned long>(serial2TxBytes));
    serial2TxBytes = 0;
    lastReport = now;
  }
#endif
}


#define SERIAL2_RX2_PIN 6
#define SERIAL2_TX2_PIN 5

void Serial2Q_begin() {
  Serial2.begin(115200, SERIAL_8N1, SERIAL2_RX2_PIN, SERIAL2_TX2_PIN);      // UART connected to the computer console, target of the pass thru
  Serial2Q.begin(&Serial2, 2048, 2048);
}


//
