
Arduino ESP32S3 project
. wifi
. BLE (bluetooth low energy)
. uart

Fan controller:
. Install "Adafruit SHT31 Library" through the Arduino Library Manager (and its listed dependencies).
. Connect the SHT31 SDA to GPIO1 and SCL to GPIO2 (default I2C address 0x44).
. Connect the fan tachometer signal to GPIO4 and its PWM control to GPIO7.
. The controller uses 25 kHz PWM, assumes two tach pulses per revolution, and starts with a 100% fan output as a fail-safe.
. The default temperature/PWM points are 25 C/20%, 35 C/40%, 45 C/65%, and 55 C/90%. Set the `Fan curve JSON` option on `/setup` to a JSON array of `[temperatureC, pwmPercent]` pairs (2-8 points); for example, `[[25,20],[35,40],[45,65],[55,90]]`. The saved curve is loaded during `fanController.begin()`; invalid or missing values leave the built-in defaults in place. Temperatures must strictly increase and PWM percentages must be nondecreasing in the 0-100 range. Values between points are linearly interpolated and values outside the curve use the nearest endpoint.
. Call `fanController.setTachPulsesPerRevolution(...)` before `begin()` if the fan uses a different tach pulse count. Temperature, tach frequency, RPM, and applied PWM are available through the class getters.
. Ensure the GPIO4 tach signal is level-shifted or pulled up to a safe 3.3 V logic level.

Arduino legacy IDE 1.8...

goal:
. use AsyncFsWebServer to provide captive portal for wifi config, and web access to files for easy way to configure 
. connect two gpio pins to the 3.3V uart (TX, RX) of a device that uses that uart as its boot/bios/uboot console.  
  For example, any linux, arm based router/nas/... box that uses the uart as its access to its boot console (uboot and later linux login).
  This connection is "Serial2"
. the main serial connection via usb is "Serial"
. add a BLE connection to emulate a serial port - this object is "btser". It advertises as `esp32ser` and exposes the Nordic UART Service (NUS) over BLE GATT.
  Use a BLE terminal client that supports NUS; this is not Bluetooth Classic SPP and does not create a Windows COM port for PuTTY or KiTTY.
. add a WiFi connection, called "webser".  This will serve a webpage with terminal emulator javascript code that uses websockets to send keystrokes/text
  back and forth.
. All 4 serial objects (Serial, Serial2, btser, webser) use the SerialQ class as a queue buffer.
. Serial2 is the 'server' port, the other 3 are the 'client' ports
. sertie class ties them together:
  - any data received from Serial2 (the device console) is sent to all 3 client ports.
  - any data received from any of the client ports is sent to Serial2
. This then makes the ESP32S3 supermini replace a USB-uart module like the CP2102 module

