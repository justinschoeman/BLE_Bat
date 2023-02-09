#ifndef _ARDUINOBLE_H_
#define _ARDUINOBLE_H_

#include <ArduinoBLE.h>

class bleUART {
public:
  bleUART(const char * addr_, const char * svcUUID_, const char * rdUUID_, const char * wrUUID_) :
    addr(String(addr_)), 
    svcUUID(String(svcUUID_)), 
    rdUUID(String(rdUUID_)), 
    wrUUID(String(wrUUID_)),
    found(false),
    connected(false),
    reconnect_time(0)
  {}

  ~bleUART(void) { close(); }

  // ble helper - check if we are the advertised device
  // return true if we are, otherwise false
  bool discoverDevice(BLEDevice dev_);

  // ble helper - return true if we have already been discovered, otherwise false
  bool isDiscovered(void) { return found; }

  // UART interface:

  // return a unique identifier string for logging
  String &myId(void) { return addr; }

  // open the device and all required characteristics
  bool open(void);

  // close device and release all resources
  void close(void);

  // return true if device is open, otherwise, false
  bool isConnected(void) { return connected; }

  // run device state engine, and return:
  // -1 = error, 0 = device not ready but not an error, 1 = OK
  int run(void);

  // read
  int read(uint8_t * buf, int len);

  // write
  int write(uint8_t * buf, int len);

private:
  String addr;
  String svcUUID;
  String rdUUID;
  String wrUUID;
  BLEDevice device;
  BLECharacteristic rdCHAR;
  BLECharacteristic wrCHAR;
  bool connected;
  bool found;
  unsigned long reconnect_time;
};

#endif
