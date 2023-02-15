#ifndef _BAT_BMS_H_
#define _BAT_BMS_H_

/*
 * Abstract interface definition for a BMS driver
 */

#include <Arduino.h>

class batBMS {
public:
  // Return a human readable identifier for this BMS instance
  virtual String &myId(void) = 0;

  // Arduiono loop() equivalent - call regularly in loop function to allow 
  // BMS to do any internal processing
  // Return code 0/1 should largely be ignored - 1 basically means that it
  // did something this time, 0 means no internal state changed.
  // -1 = error, 0 = run again, 1 = OK
  virtual int run(void) = 0;

  // Tell this BMS driver to poll the associated BMS for updated data
  // return true if a poll message was successfully sent
  virtual bool poll(void) = 0;

  // test if poll operation is still running (keep calling run() until complete...)
  virtual bool isBusy(void) = 0;
};

#endif
