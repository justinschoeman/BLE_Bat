#include "batDaly.h"

// RX
// A5 1 90 8 2 F 0 0 75 38 2 93 91
//  | |  | | | | | |  |  | |  |  |
//  | |  | | | | | |  |  | |  |  - CRC
//  | |  | | | | | |  |  | |  ---- data 7
//  | |  | | | | | |  |  | ------- data 6
//  | |  | | | | | |  |  --------- data 5
//  | |  | | | | | |  ------------ data 4
//  | |  | | | | | --------------- data 3
//  | |  | | | | ----------------- data 2
//  | |  | | | ------------------- data 1
//  | |  | | --------------------- data 0
//  | |  | ----------------------- nr of data bytes (8 for all frames we are interested in)
//  | |  ------------------------- original command
//  | ---------------------------- device address (always 1?)
//  ------------------------------ start sentinel
// in theory all frames will be fixed 13 bytes long

void batDaly::logTag(const char *tag) {
  Serial.print(myId());
  Serial.print(" : ");
  Serial.print(tag);
  Serial.print(" = ");
}

void batDaly::log(const char *tag, int value, int fmt) {
  //return;
  logTag(tag);
  Serial.println(value, fmt);
}

void batDaly::log(const char *tag, double value, int fmt) {
  //return;
  logTag(tag);
  Serial.println(value, fmt);
}

// -1 = error, 0 = run again, 1 = OK
int batDaly::run(void) {
  if (errors > 5) {
    Serial.println("TOO MANY ERRORS - RESTART UART");
    uart->close();
    errors = 0;
  }
  int i = uart->run();
  if (i < 0) {
    Serial.println("Error in uart->run()");
    errors++;
    return -1;
  }
  if (i == 0) {
    errors = 0;
    return 0;
  }
  // check for read....
  i = uart->read(buf + rxo, sizeof(buf) - rxo);
  if(i < 0) {
    Serial.println("Read failed!");
    errors++;
    return -1;
  }
  while(i != 0) {  // fake loop so we can break when needed
    Serial.print("Read ");
    Serial.print(myId());
    Serial.print(": ");
    Serial.println(i);
    for(int j = 0; j < i; j++) {
      Serial.print(buf[rxo + j], HEX);
      Serial.print(" ");
    }
    Serial.println();
    if(!runstate) {
      Serial.println("Unsollicited message - ignore.");
      break;
    }
    // check basic framing...
    if(rxo) {
      // receiving additional data on a partial buffer
      if(buf[rxo] == 0xa5) {
        Serial.println("offset packet starts with start sentinel - discard old data and use this...");
        memmove(buf, &buf[rxo], i);
        rxo = 0;
      }
    } else {
      // entire frame
      if(buf[0] != 0xa5) {
        Serial.println("Invalid start sentinel - discard data!");
        break;
      }
    }
    // check length
    i += rxo;  // total buffer length
    if(i < 4) {
      Serial.println("too short - wait for more data");
      rxo = i;
      break;
    }
    // data length
    int len = buf[3];
    if(len != 8) {
      Serial.print("Invalid data length: ");
      Serial.println(len);
      errors++;
      runstate = 0;
      return -1;
    }
    if(i < len + 5) {
      Serial.println("too short for length - wait for more data");
      rxo = i;
      break;
    }
    //Serial.println("process response");
    if(!handleRx(i)) {
      Serial.println("Bad response!");
      errors++;
      runstate = 0;
      return -1;
    }
    i -= len + 5;
    Serial.print("remainder: ");
    Serial.println(i);
    if(i > 0) {
      // fake a read of the remaining bytes
      memmove(buf, buf + len + 5, i);
      rxo = 0;
      continue;      
    }
    break;  // end fake loop
  }
  if (runstate) {
    // still in runstate? check rx timeout!
    if (millis() - runtime > 500UL) {
      Serial.println("READ TIMEOUT!");
      errors++;
      runstate = 0;
      return -1;
    }
  }
  return 1;
}

// return true if a poll message was successfully sent
bool batDaly::poll(void) {
  if (runstate) {
    Serial.println("POLL IN BUSY STATE! IGNORE");
    return true;
  }
  if (!sendCommand(0x90)) {
    Serial.println("send 0x90 failed!");
    errors++;
    return false;
  }
  runstate = 1;
  return true;
}

uint8_t batDaly::calcCRC(uint8_t *b, int len) {
  uint8_t crc = 0;
  for (int i = 0; i < len; i++) {
    crc += b[i];
  }
  return crc;
}

bool batDaly::sendCommand(uint8_t cmd) {
  //uint8_t txbuf[] = { 0xA5, 0x40, 0x90, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7D };
  txbuf[2] = cmd;
  txbuf[12] = calcCRC(txbuf, 12);
  Serial.print("TX: ");
  for (int i = 0; i < sizeof(txbuf); i++) {
    Serial.print(txbuf[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  int i = uart->write(txbuf, sizeof(txbuf));
  if (i != sizeof(txbuf)) {
    Serial.println("write failed!");
    errors++;
    return false;
  }
  rxo = 0;  // start with a clean rx buffer...
  errors = 0;
  runtime = millis();
  return true;
}

uint8_t batDaly::get8(void) {
  return buf[bo++];
}

uint16_t batDaly::get16(void) {
  uint16_t r = buf[bo++];
  r <<= 8;
  return r | buf[bo++];
}

uint32_t batDaly::get32(void) {
  uint32_t r = get16();
  r <<= 16;
  return r | get16();
}

bool batDaly::handleRx(int len) {
  if (len < 13) {
    Serial.println("too short!");
    return false;
  }
  // crc
  uint8_t msgCRC = buf[12];
  uint8_t dataCRC = calcCRC(buf, 12);
  if (msgCRC != dataCRC) {
    Serial.println("BAD CRC!");
    Serial.print("msgCRC: ");
    Serial.println(msgCRC, HEX);
    Serial.print("dataCRC: ");
    Serial.println(dataCRC, HEX);
    return false;
  }
  // now start scanning from the top...
  bo = 0;
  // 0 = start sentinel (already checked)
  int i = get8();
  // 1 = device address
  i = get8();
  if (i != dev_addr) {
    Serial.print("INVALID DEVICE ADDRESS: ");
    Serial.println(i);
    Serial.print("SHOUND BE: ");
    Serial.println(dev_addr);
    return false;
  }
  // 2 = command
  uint8_t cmd = get8();
  Serial.print("COMMAND: ");
  Serial.println(cmd, HEX);
  // 3 = data length
  i = get8();
  if (i != 8) {
    Serial.print("INVALID LENGTH (should be): ");
    Serial.println(8);
    Serial.print("Length: ");
    Serial.println(i);
    return false;
  }
  if (cmd == 0x90) return do90();
  if (cmd == 0x91) return do91();
  if (cmd == 0x92) return do92();
  if (cmd == 0x93) return do93();
  if (cmd == 0x94) return do94();
  if (cmd == 0x95) return do95();
  if (cmd == 0x96) return do96();
  if (cmd == 0x97) return do97();
  if (cmd == 0x98) return do98();
  Serial.println("BAD COMMAND!");
  return false;
}

// 90 and 91 are critical - poll the others in sequence...
bool batDaly::do90(void) {
  int i;
  float f;
  // 0..1 = voltage
  f = get16();
  f *= 0.1f;
  log("voltage", f);
  bat->voltage = f;
  // 2..3 = acquisition?
  f = get16();
  f *= 0.1f;
  log("acquisition?", f);
  // 4..5 = current
  f = get16();
  f -= 30000.0f;
  f *= 0.1f;
  log("current", f);
  bat->current = f;
  // 6..7 = soc
  f = get16();
  f *= 0.1f;
  log("soc", f);
  bat->soc = f;
  errors = 0;
  //runstate = 0;
  sendCommand(0x91);
  return true;
}

bool batDaly::do91(void) {
  int i;
  float f;
  // 0..1 = maxvoltage
  f = get16();
  f *= 0.001f;
  log("maxCellVoltage", f, 3);
  bat->maxCellVoltage = f;
  // 2 = maxCellVoltageNumber
  i = get8();
  i -= 1;
  log("maxCellVoltageNumber", i);
  bat->maxCellVoltageNumber = i;
  // 3..4 = minvoltage
  f = get16();
  f *= 0.001f;
  log("minCellVoltage", f, 3);
  bat->minCellVoltage = f;
  // 5 = minCellVoltageNumber
  i = get8();
  i -= 1;
  log("minCellVoltageNumber", i);
  bat->minCellVoltageNumber = i;
  errors = 0;
  bat->updateMillis = millis(); // already have all the critical bits - bump the timestamp now
  //runstate = 0;
  sendCommand(pollSeq[pollNum++]);
  if(pollNum >= sizeof(pollSeq)) pollNum = 0;
  // test
  //Serial.println(myId());
  //bat->dump();
  return true;
}

// unit numbers are not cells - arb temp sensor numbers
bool batDaly::do92(void) {
  int i;
  float f;
  // 0 = maxTemp
  f = get8();
  f -= 40.0f;
  log("maxTemp", f, 3);
  bat->temperature = f;
  // 1 = maxCellTempNumber
  i = get8();
  i -= 1;
  log("maxCellTempNumber", i);
  //bat->maxCellVoltageNumber = i;
  // 2 = minTemp
  f = get8();
  f -= 40.0f;
  log("minCellTemp", f, 3);
  bat->temperature += f;
  bat->temperature /= 2.0f;
  // 3 = minCellTempNumber
  i = get8();
  i -= 1;
  log("minCellTempNumber", i);
  //bat->minCellVoltageNumber = i;
  errors = 0;
  runstate = 0;
  return true;
}

// purely informational - probably not worth capturing
// even residual capacity is exactly AH*SOC
bool batDaly::do93(void) {
  int i;
  // 0 = charge/discharge status (0 stationary ,1 charged ,2 discharged)
  i = get8();
  log("charge/discharge status", i, HEX);
  // 1 = charging MOS tube status
  i = get8();
  log("charging MOS tube status", i, HEX);
  // 2 = discharge MOS tube state
  i = get8();
  log("discharge MOS tube state", i, HEX);
  // 3 = BMS life(0~255 cycles)
  i = get8();
  log("BMS life", i);
  // 4..7 = residual capacity (mAH)  
  i = get32();
  log("residual capacity", i);
  //bat->maxCellVoltage = f;
  errors = 0;
  runstate = 0;
  return true;
}

// also not very useful...
bool batDaly::do94(void) {
  int i;
  // 0 = battery string
  i = get8();
  log("numCells", i);
  if(i != bat->numCells) {
    Serial.print("!!! DALY REPORTS DIFFERENT CELL COUNT TO US: ");
    Serial.println(i);
  }
  // 1 = numTemps
  i = get8();
  log("numTemps", i);
  // 2 = charger status (0 disconnected ,1 connected)
  i = get8();
  log("charger status", i);
  // 3 = load status (0 disconnected ,1 access)
  i = get8();
  log("load status", i);
  // 4 = di/do status
  i = get8();
  log("di/do status", i);
  // 5..6 = charge/discharge cycles  
  i = get16();
  log("cycles", i);
  errors = 0;
  runstate = 0;
  return true;
}

// eeewwwww.... it sends all 16 cells in consecutive packets of 3 cells each
// takes 1.5s!!!! don't do this too often, as we will overrun polls...
bool batDaly::do95(void) {
  int i;
  float f;
  // 0 = frame
  i = get8();
  log("frame", i);
  int nrFrames = (bat->numCells + 2) / 3;
  log("nrFrames", nrFrames);
  int base = (i - 1) * 3;
  log("base", base);
  for(int j = 0; j < 3 && base < bat->numCells; j++, base++) {
    // 1..2 = voltage
    f = get16();
    f *= 0.001f;
    log("Voltage", f, 3);
    bat->cells[base].voltage = f;
  }
  errors = 0;
  if(i < nrFrames) {
    runtime = millis();
    return true; // read some more...
  }
  runstate = 0;
  // test
  Serial.println(myId());
  bat->dump();
  return true;
}

// 96 is useless - no way to tell up front how many frames there will be
// and have not seen one with more than 2 temps (already reported)
// SKIP!
bool batDaly::do96(void) {
  int i;
  float f;
  // 0 = frame
  i = get8();
  log("frame", i);
  for(int j = 0; j < 6; j++) {
    f = get8();
    f -= 40.0f;
    log("maxCellVoltage", f, 3);
    bat->maxCellVoltage = f;
  }
  errors = 0;
  runstate = 0;
  return true;
}

bool batDaly::do97(void) {
  for(int j = 0; j < 8; j++) {
    int i = get8();
    log("bal", i, HEX);    
  }
  errors = 0;
  runstate = 0;
  return true;
}

bool batDaly::do98(void) {
  for(int j = 0; j < 8; j++) {
    int i = get8();
    log("bitfields", i, HEX);
  }
  errors = 0;
  runstate = 0;
  return true;
}

/*
  float soh;
*/