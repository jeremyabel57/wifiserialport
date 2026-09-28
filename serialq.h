/*
  stream queue

  and object instance SerialQ operating the Serial object
  
*/

#ifndef SERIALQ_H
#define SERIALQ_H

#define SERIALQ SerialQ                         // for use as SERIALQ.function() instead of SerialQ.function().  then if not including SerialQ then define SERIALQ as Serial


#include <Arduino.h>                            // Arduino
#include <inttypes.h>                           // inttypes
#include "queuearray.h"                         // queue implemented in an array
#include "stdarg.h"                             // variable argument functions, like printf


void SerialQ_begin();                           // initialize SerialQ connected to the Serial Object, baud 115200


class SerialQc {
 
public:
  SerialQc();                                   // constructor - set max length of queue (0-254), allocate buffer memory
  ~SerialQc();                                  // destructor - free buffer memory

  void begin(Stream *s1 = &Serial, uint16_t bin = 256, uint16_t bout = 256);    // set the size of the buffers
  void end();                                                                   // flush output queue to get ready for shutdown
  void loop();                                                                  // run inside main loop

  void flushout(uint16_t n = 0);                // flush the output queue, writing the queue data to the stream
  void flushin(uint16_t n = 0);                 // flush the input queue, discarding the input data

  uint16_t available();                         // number of bytes available in the queue or the stream
  char read();                                  // read one byte from the queue/stream
  uint16_t read(char *buf, uint16_t len);       // read a block of data, return number of characters read, nonblocking
  uint16_t read(uint8_t *buf, uint16_t len);    // read a block of data, return number of characters read, nonblocking

  // write to queue/stream
  void write(char);                             // single character
  void write(const char *);                     // string
  uint16_t write(const char *, uint16_t);       // buffer and length
  uint16_t write(const uint8_t *, uint16_t);    // buffer and length
  void print(const char);                       // char
  void print(const char *);                     // string
  void print(const uint8_t);                    // uint8_t
  void print(const uint16_t);                   // uint16_t
  void print(const uint32_t);                   // uint32_t
  void print(const int8_t);                     // int8_t
  void print(const int16_t);                    // int16_t
  void print(const int32_t);                    // int32_t
  void print(const long double);                // long long float
  void print(const float);                      // float
  void print(const double);                     // double
  void println(const char *);                   // string + newline
  void println();                               // just the newline
  int printfv(const char *fmt, va_list args);   // formatted string
  int printf(const char *fmt, ...);             // formatted string


protected:
  QueueArray<char, uint16_t> qin,               // input queue
    qout;                                       // output queue
  Stream *s;                                    // i/o stream

  virtual uint16_t s_available();               // check stream for available data
  virtual char s_read();                        // read one character from the stream
  virtual void s_write(char);                   // write one character to the stream
 
};



#ifdef SERIALQ_C
SerialQc SerialQ;
#else
extern SerialQc SerialQ;                        // instanciation of serial Q on object Serial
#endif


#endif

//
