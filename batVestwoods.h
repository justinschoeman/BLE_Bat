#ifndef _BATVESTWOODS_H_
#define _BATVESTWOODS_H_

#include "bleUART.h"

class batVestwoods {
public:
  batVestwoods(bleUART * uart_) :
    uart(uart_),
    runstate(0),
    errors(0),
    rxo(0)
  {}

  String &myId(void) { return uart->myId(); }

  // -1 = error, 0 = run again, 1 = OK
  int run(void);

  // fixme... -1 on error, 0 on still busy, 1 on poll complete
  int poll(void);

  // test if poll operation is still running
  bool isBusy(void) { return runstate ? 1 : 0; }

private:
  uint16_t calcCRC(uint8_t * b, int len);
  uint8_t get8(void);
  uint16_t get16(void);
  bool handleRx(int len);
  bool do0001(void);

  bleUART * uart;
  int runstate; // 0 = idle, 1 = waiting for rx
  unsigned long runtime;
  uint8_t buf[256];
  uint8_t rxo; // rx offset into buf
  uint8_t bo; // scan offset into buf...
  int errors; // error count
};

#endif