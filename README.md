
Arduino ESP32S3 project
. wifi
. BLE (bluetooth low energy)
. uart

Arduino legacy IDE 1.8...

goal:
. use AsyncFsWebServer to provide captive portal for wifi config, and web access to files for easy way to configure 
. connect two gpio pins to the 3.3V uart (TX, RX) of a device that uses that uart as its boot/bios/uboot console.  
  For example, any linux, arm based router/nas/... box that uses the uart as its access to its boot console (uboot and later linux login).
  This connection is "Serial2"
. the main serial connection via usb is "Serial"
. add a BLE connection to emulate a serial port - this object is "btser".  This will give a cell phone or computer with BT the ability to
  access it and connect a serial terminal to it (i.e. PuTTY or KiTTY)
. add a WiFi connection, called "webser".  This will serve a webpage with terminal emulator javascript code that uses websockets to send keystrokes/text
  back and forth.
. All 4 serial objects (Serial, Serial2, btser, webser) use the SerialQ class as a queue buffer.
. Serial2 is the 'server' port, the other 3 are the 'client' ports
. sertie class ties them together:
  - any data received from Serial2 (the device console) is sent to all 3 client ports.
  - any data received from any of the client ports is sent to Serial2
. This then makes the ESP32S3 supermini replace a USB-uart module like the CP2102 module

