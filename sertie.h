


#ifndef SERTIE_H
#define SERTIE_H


/*

sertie.begin(); 	// setup
sertie.loop();	  // interface with host thru usb/primary uart


*/

#include <Arduino.h>     //


class sertiec {

public:

  void begin();               // initialize (setup)
  void loop();                // run in loop to respond to serial input

};


#ifdef SERTIE_C
class sertiec sertie;
#else
extern class sertiec sertie;  // serial interface object
#endif


#endif

//
