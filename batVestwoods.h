#ifndef _BATVESTWOODS_H_
#define _BATVESTWOODS_H_

/*
 * Vestwoods specific implementation of batBMS
 */

#include "batBMS.h"
#include "batUART.h"
#include "bat.h"

class batVestwoods : public batBMS {
public:
  batVestwoods(batUART *uart_, batBat * bat_)
    : uart(uart_),
      bat(bat_),
      runstate(0),
      errors(0),
      rxo(0) {}

  String &myId(void) {
    return uart->myId();
  }

  void logTag(const char *tag);
  void log(const char *tag, int value, int fmt = DEC);
  void log(const char *tag, double value, int fmt = 2);

  // -1 = error, 0 = run again, 1 = OK
  int run(void);

  // return true if a poll message was successfully sent
  bool poll(void);

  // test if poll operation is still running
  bool isBusy(void) {
    return runstate ? 1 : 0;
  }

private:
  uint16_t calcCRC(uint8_t *b, int len);
  uint8_t get8(void);
  uint16_t get16(void);
  bool handleRx(int len);
  bool do0001(void);

  batUART * uart;
  batBat * bat;
  int runstate;  // 0 = idle, 1 = waiting for rx
  unsigned long runtime;
  uint8_t buf[256];
  uint8_t rxo;  // rx offset into buf
  uint8_t bo;   // scan offset into buf...
  int errors;   // error count
};

#endif