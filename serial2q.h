


#ifndef SERIAL2Q_H
#define SERIAL2Q_H


/*

Serial2Q.begin();	// setup


*/


#define SERIAL2Q Serial2Q                       // for use as SERIALQ.function() instead of SerialQ.function().  then if not including SerialQ then define SERIALQ as Serial


#include <Arduino.h>                            // Arduino
#include <inttypes.h>                           // inttypes
#include "serialq.h"                            //



#ifndef SERIAL2Q_C
extern class SerialQc Serial2Q;  // serial interface object
#endif



void Serial2Q_begin();          // for main code to call this in the begin function


#endif

//
