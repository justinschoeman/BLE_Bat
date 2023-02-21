#ifndef _BATDALY_H_
#define _BATDALY_H_

/*
 * Vestwoods specific implementation of batBMS
 */

#include "batBMS.h"
#include "batUART.h"
#include "bat.h"

// in theory host_addr should be different based on channel, but 0x40 seems to work for everything?
// BMS主控BMS Master  0x01
// 蓝牙手机APP Bluetooth APP  0x20
// GPRS 0x40
// 上位机Upper  0x80
// as far as I can tell, dev_addr is always 0x01
class batDaly : public batBMS {
public:
  batDaly(batUART *uart_, batBat * bat_, uint8_t host_addr_ = 0x40, uint8_t dev_addr_ = 0x01)
    : uart(uart_),
      bat(bat_),
      host_addr(host_addr_),
      dev_addr(dev_addr_),
      runstate(0),
      errors(0),
      rxo(0),
      pollNum(0) {
    memset(txbuf, 0, sizeof(txbuf));
    txbuf[0] = 0xa5;
    txbuf[1] = host_addr_;
    txbuf[3] = 8;
  }

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
  uint8_t calcCRC(uint8_t *b, int len);
  bool sendCommand(uint8_t cmd);
  uint8_t get8(void);
  uint16_t get16(void);
  uint32_t get32(void);
  bool handleRx(int len);
  bool do90(void);
  bool do91(void);
  bool do92(void);
  bool do93(void);
  bool do94(void);
  bool do95(void);
  bool do96(void);
  bool do97(void);
  bool do98(void);

  batUART * uart;
  batBat * bat;
  uint8_t host_addr;
  uint8_t dev_addr;
  int runstate;  // 0 = idle, 1 = waiting for rx
  unsigned long runtime;
  uint8_t txbuf[13];
  uint8_t buf[256];
  uint8_t rxo;  // rx offset into buf
  uint8_t bo;   // scan offset into buf...
  int errors;   // error count
  uint8_t pollSeq[6] = {0x92, 0x93, 0x94, 0x95, /*0x96, */0x97, 0x98};
  int pollNum;
};

#endif