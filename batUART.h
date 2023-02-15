#ifndef _BAT_UART_H_
#define _BAT_UART_H_

/*
 * Abstract interface definition for a UART driver
 */

#include <Arduino.h>

class batUART {
public:
  // return a unique human readable identifier string for logging
  virtual String &myId(void) = 0;

  // open the device and all required characteristics
  // typically not called directly - run() will try to make sure the device is open
  virtual bool open(void) = 0;

  // close device and release all resources
  virtual void close(void) = 0;

  // return true if device is open, otherwise, false
  virtual bool isConnected(void) = 0;

  // run device state engine, and return:
  // arduino style loop function - call repeatedly to allow the driver to do its processing
  // -1 = error, 0 = device not ready but not an error, 1 = OK
  virtual int run(void) = 0;

  // read
  // read up to len bytes into buf
  // *** some drivers do not allow multiple reads - make sure buf is big enough for target data!
  // *** some drivers chunk data - call repeatedly until you have all data!
  // -1 = error, 0 = nothing to read, >0 length of data read
  virtual int read(uint8_t * buf, int len) = 0;

  // write
  // attempt to write len bytes of buffer to device
  // may not write all bytes in one go - call repeatedly to empty buffer...
  // -1 = error, otherwise number of bytes written
  virtual int write(uint8_t * buf, int len) = 0;
};

#endif
