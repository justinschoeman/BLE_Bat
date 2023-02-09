#include "batVestwoods.h"

uint16_t batVestwoods::calcCRC(uint8_t * b, int len) {
  if(!b || len <= 0) return 0;
  uint16_t crc = 0xFFFF;
  for(int i = 0; i < len; i++) {
    crc ^= b[i];
    for(int j = 0; j < 8; j++) {
      if(crc & 1) {
        crc = (crc >> 1) ^ 40961;
      } else {
        crc = crc >> 1;
      }
    }
  }
  return crc & 0xFFFF;
}

uint8_t batVestwoods::get8(void) {
  return buf[bo++];
}

uint16_t batVestwoods::get16(void) {
  uint16_t r = buf[bo++];
  r <<= 8;
  return r | buf[bo++];
}

bool batVestwoods::handleRx(int len) {
  if(len < 8) {
    Serial.println("too short!");
    return false;
  }
  // end sentinel
  if(buf[len - 1] != 0xa7) {
    Serial.println("end sentinel invalid!");
    return false;
  }
  // crc
  uint16_t msgCRC = buf[len - 3];
  msgCRC <<= 8;    
  msgCRC |= buf[len - 2];
  uint16_t dataCRC = calcCRC(buf + 1, len - 4);
  if(msgCRC != dataCRC) {
    Serial.println("BAD CRC!");
    Serial.print("msgCRC: ");
    Serial.println(msgCRC, HEX);
    Serial.print("dataCRC: ");
    Serial.println(dataCRC, HEX);
    return false;
  }
  // now start scanning from the top...
  bo = 0;
  // start sentinel
  if(get8() != 0x7a) {
    Serial.println("start sentinel invalid!");
    return false;
  }
  // 1 = unknwon - probably high byte of length
  int i = get8();
  // 2 = length
  i = get8();
  if(len != i + 4) {
    Serial.print("INVALID LENGTH (should be): ");
    Serial.println(len);
    Serial.print("Length: ");
    Serial.println(i);
    return false;
  }
  // 4 = unknown - probably address
  i = get8();
  // 5,6 = command
  i = get16();
  Serial.print("command: ");
  Serial.println(i, HEX);
  if(i == 0x0001) return do0001();
  Serial.println("BAD COMMAND!");
  return false;
}

bool batVestwoods::do0001(void) {
  return true;
}