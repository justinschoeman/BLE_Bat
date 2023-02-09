#ifndef _BATVESTWOODS_H_
#define _BATVESTWOODS_H_

#include "bleUART.h"

class batVestwoods {
public:
  batVestwoods(bleUART * uart_) :
    uart(uart_),
    runstate(0)
  {}

  String &myId(void) { return uart->myId(); }

  // -1 = error, 0 = run again, 1 = OK
  int run(void) {
    int i = uart->run();
    if(i < 0) {
      Serial.println("Error in uart->run()");
      return -1;
    }
    if(i == 0) return 0;
    // check for read....
    i = uart->read(buf, sizeof(buf));
    if(i < 0) {
      Serial.println("Read failed!");
      return -1;
    }
    if(i != 0) {
      Serial.print("Read ");
      Serial.print(myId());
      Serial.print(": ");
      Serial.println(i);
      for(int j = 0; j < i; j++) {
        Serial.print(buf[j], HEX);
        Serial.print(" ");
      }
      Serial.println();
      if(runstate) {
        //Serial.println("process response");
        if(!handleRx(i)) {
          Serial.println("Bad response!");
        }
        runstate = 0;
      } else {
        Serial.println("Unsollicited message - ignore.");
      }
    }
    if(runstate) {
      // still in runstate? check rx timeout!
      if(millis() - runtime > 500UL) {
        Serial.println("READ TIMEOUT!");
        // fixme - count timeouts and restart device...
        uart->close();
        runstate = 0;
        return -1;
      }
    }
    return 1;
  }

  // fixme... -1 on error, 0 on still busy, 1 on poll complete
  int poll(void) {
    if(runstate) {
      Serial.println("POLL IN BUSY STATE! IGNORE");
      return true;
    }
    uint8_t txbuf[] = {0x7a, 0, 5, 0, 0, 1, 12, 229, 0xa7};
    int i = uart->write(txbuf, sizeof(txbuf));
    //Serial.print("Wrote: ");
    //Serial.println(i);
    if(i != sizeof(txbuf)) {
      Serial.println("write failed!");
      uart->close();
      return false;
    }
    runstate = 1;
    runtime = millis();
    return true;
  }

  bool isBusy(void) {
    return runstate ? 1 : 0;
  }

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
  uint8_t bo; // scan offset into buf...
};

#endif