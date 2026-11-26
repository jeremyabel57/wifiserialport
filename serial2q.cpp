
#define SERIAL2Q_C
#include "serial2q.h"


/*
  void Serial2Qc::begin(Stream *s1, uint16_t bin, uint16_t bout) {
  SerialQc::begin(s1, bin, bout);       // initialize the base/inherited class items
  s1->begin(115200, SERIAL_8N1, SERIAL2_RX2_PIN, SERIAL2_TX2_PIN);      // UART connected to the computer console, target of the pass thru
  }
*/


class SerialQc Serial2Q;


//???
//#define SERIAL2_RX2_PIN 5
//#define SERIAL2_TX2_PIN 4

//???
#define SERIAL2_RX2_PIN 6
#define SERIAL2_TX2_PIN 5

void Serial2Q_begin() {
  Serial2.begin(115200, SERIAL_8N1, SERIAL2_RX2_PIN, SERIAL2_TX2_PIN);      // UART connected to the computer console, target of the pass thru
  Serial2Q.begin(&Serial2, 2048, 2048);
}


//
