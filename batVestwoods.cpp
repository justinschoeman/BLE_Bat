#include "batVestwoods.h"

void batVestwoods::logTag(const char *tag) {
  Serial.print(myId());
  Serial.print(" : ");
  Serial.print(tag);
  Serial.print(" = ");
}

void batVestwoods::log(const char *tag, int value, int fmt) {
  return;  
  logTag(tag);
  Serial.println(value, fmt);
}

void batVestwoods::log(const char *tag, double value, int fmt) {
  return;
  logTag(tag);
  Serial.println(value, fmt);
}

// -1 = error, 0 = run again, 1 = OK
int batVestwoods::run(void) {
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
  if (i < 0) {
    Serial.println("Read failed!");
    errors++;
    return -1;
  }
  while (i != 0) {  // fake loop so we can break when needed
    Serial.print("Read ");
    Serial.print(myId());
    Serial.print(": ");
    Serial.println(i);
    for (int j = 0; j < i; j++) {
      Serial.print(buf[rxo + j], HEX);
      Serial.print(" ");
    }
    Serial.println();
    if (!runstate) {
      Serial.println("Unsollicited message - ignore.");
      break;
    }
    // check basic framing...
    if (rxo) {
      // receiving additional data on a partial buffer
      if (buf[rxo] == 0x7a) {
        Serial.println("offset packet starts with start sentinel - discard old data and use this...");
        memmove(buf, &buf[rxo], i);
        rxo = 0;
      }
    } else {
      // entire frame
      if (buf[0] != 0x7a) {
        Serial.println("Invalid start sentinel - discard data!");
        break;
      }
    }
    // check end sentinel
    i += rxo;  // total buffer length
    if (i < 8) {
      Serial.println("too short - wait for more data");
      rxo = i;
      break;
    }
    int len = buf[2];
    if (i < len + 4) {
      Serial.println("too short for length - wait for more data");
      rxo = i;
      break;
    }
    if (buf[1 + len + 2] != 0xa7) {
      // no end sentinel?
      Serial.println("No end sentinel - wait for more data...");
      rxo = i;
      break;
    }
    //Serial.println("process response");
    if (!handleRx(rxo + i)) {
      Serial.println("Bad response!");
    }
    errors = 0;
    runstate = 0;
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

// fixme... -1 on error, 0 on still busy, 1 on poll complete
int batVestwoods::poll(void) {
  if (runstate) {
    Serial.println("POLL IN BUSY STATE! IGNORE");
    return true;
  }
  uint8_t txbuf[] = { 0x7a, 0, 5, 0, 0, 1, 12, 229, 0xa7 };
  int i = uart->write(txbuf, sizeof(txbuf));
  //Serial.print("Wrote: ");
  //Serial.println(i);
  if (i != sizeof(txbuf)) {
    Serial.println("write failed!");
    errors++;
    return false;
  }
  runstate = 1;
  rxo = 0;  // start with a clean ex buffer...
  errors = 0;
  runtime = millis();
  return true;
}

uint16_t batVestwoods::calcCRC(uint8_t* b, int len) {
  if (!b || len <= 0) return 0;
  uint16_t crc = 0xFFFF;
  for (int i = 0; i < len; i++) {
    crc ^= b[i];
    for (int j = 0; j < 8; j++) {
      if (crc & 1) {
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
  if (len < 8) {
    Serial.println("too short!");
    return false;
  }
  // crc
  uint16_t msgCRC = buf[len - 3];
  msgCRC <<= 8;
  msgCRC |= buf[len - 2];
  uint16_t dataCRC = calcCRC(buf + 1, len - 4);
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
  // start sentinel
  int i = get8();
  // 1 = unknwon - probably high byte of length
  i = get8();
  // 2 = length
  i = get8();
  if (len != i + 4) {
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
  if (i == 0x0001) return do0001();
  Serial.println("BAD COMMAND!");
  return false;
}

bool batVestwoods::do0001(void) {
  int i;
  float f;
  // 0 - onlineStatus
  i = get8();
  log("onlineStatus", i);
  // 1 - batteriesSeriesNumber
  i = get8();
  log("batteriesSeriesNumber", i);
  if(i != bat->numCells) { Serial.println("WARNING BMS REPORTS DIFFERENT CELL COUNT THAN OUR CONFIGURED BATTERY!"); }
  for (int j = 0; j < i; j++) {
    // 2, 3 - cellVoltage (mV)
    f = get16();
    f /= 1000.0f;
    log("cellVoltage", f);
    if(j < bat->numCells) {
      bat->cells[j].voltage = f;
    }
  }
  // 4 - maxCellNumber
  i = get8();
  log("maxCellNumber", i);
  bat->maxCellVoltageNumber = i - 1;
  // 5,6 - maxCellVoltage
  f = get16();
  f /= 1000.0f;
  log("maxCellVoltage", f);
  bat->maxCellVoltage = f;
  // 7 - minCellNumber
  i = get8();
  log("minCellNumber", i);
  bat->minCellVoltageNumber = i - 1;
  // 8,9 - minCellVoltage
  f = get16();
  f /= 1000.0f;
  log("minCellVoltage", i);
  bat->minCellVoltage = f;
  // 10,11 - totalCurrent ( x / 100 - 300 )
  f = get16();
  f = (f / 100.0) - 300.0;
  log("totalCurrent", f);
  bat->current = f;
  // 12, 13 - soc (x / 100)
  f = get16();
  f /= 100.0;
  log("soc", f);
  bat->soc = f;
  // 14, 15 - soh (x / 100)
  f = get16();
  f /= 100.0;
  log("soh", f);
  bat->soh = f;
  // 16, 17 - actualCapacity (x / 100)
  f = get16();
  f /= 100.0;
  log("actualCapacity", f);
  // 18, 19 - surplusCapacity (x / 100)
  f = get16();
  f /= 100.0;
  log("surplusCapacity", f);
  // 20, 21 - nominalCapacity (x / 100)
  f = get16();
  f /= 100.0;
  log("nominalCapacity", f);
  // 22 - batteriesTemperatureNumber
  i = get8();
  log("batteriesTemperatureNumber", i);
  for (int j = 0; j < i; j++) {
    // 23, 24 - cellTemperature (x - 50)
    int k = get16();
    k -= 50;
    log("cellTemperature", k);
    // they do not send all temperatures, so we will populate from min/max
  }
  // 25, 26 - environmentalTemperature
  i = get16();
  i -= 50;
  log("environmentalTemperature", i);
  // 25, 26 -pcbTemperature
  i = get16();
  i -= 50;
  log("pcbTemperature", i);
  // reset cell temps..
  for(int j = 0; j < bat->numCells; j++) bat->cells[j].temperature = -99.0f;
  bat->temperature = 0;
  // 27 - maxTemperatureCellNumber
  i = get8();
  log("maxTemperatureCellNumber", i);
  // 28 - maxTemperatureCellValue
  f = get8();
  f -= 50.0f;
  log("maxTemperatureCellValue", f);
  if(i < 1 || i > bat->numCells) {
    Serial.print("bad maxTemperatureCellNumber ");
    Serial.println(i);
  } else {
    bat->cells[i - 1].temperature = f;
    bat->temperature += f;
  }
  // 29 - minTemperatureCellNumber
  i = get8();
  log("minTemperatureCellNumber", i);
  // 30 - minTemperatureCellValue
  f = get8();
  f -= 50;
  log("minTemperatureCellValue", f);
  if(i < 1 || i > bat->numCells) {
    Serial.print("bad minTemperatureCellNumber ");
    Serial.println(i);
  } else {
    bat->cells[i - 1].temperature = f;
    bat->temperature += f;
  }
  bat->temperature /= 2.0f; // average
  for(int j = 0; j < bat->numCells; j++) {
    if(bat->cells[j].temperature == -99.0f) bat->cells[j].temperature = bat->temperature;
  }
  // 31 bmsFault1
  i = get8();
  log("bmsFault1", i, HEX);
  // 32 bmsFault2
  i = get8();
  log("bmsFault2", i, HEX);
  // 33 bmsAlert1
  i = get8();
  log("bmsAlert1", i, HEX);
  // 34 bmsAlert2
  i = get8();
  log("bmsAlert2", i, HEX);
  // 35 bmsAlert3
  i = get8();
  log("bmsAlert3", i, HEX);
  // 36 bmsAlert4
  i = get8();
  log("bmsAlert4", i, HEX);
  // 37, 38 u.cycleIndex
  i = get16();
  log("cycleIndex", i);
  // 39, 40 totalVoltage ( x / 100)
  f = get16();
  f /= 100.0;
  log("totalVoltage", f);
  bat->voltage = f;
  // 41 bmsStatus
  i = get8();
  log("bmsStatus", i, HEX);
  // 42, 43 - totalChargingCapacity
  //i = get16();
  //log("totalChargingCapacity", i);
  // 44, 45 - totalDischargeCapacity
  //i = get16();
  //log("totalDischargeCapacity", i);
  // 46, 47 - totalRechargeTime
  //i = get16();
  //log("totalRechargeTime", i);
  // 48, 49 - totaldischargeTime
  //i = get16();
  //log("totaldischargeTime", i);
  // 50 - batteryType
  //i = get8();
  //log("batteryType", i);
  bat->updateMillis = millis();
  // test
  bat->dump();
  return true;
}